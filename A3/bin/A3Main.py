#!/usr/bin/env python3

import os
import sys
import ctypes
from ctypes import c_void_p, byref
from asciimatics.screen import Screen
from asciimatics.widgets import Frame, ListBox, Layout, Divider, Button, Widget, Text, PopUpDialog, TextBox
from asciimatics.exceptions import ResizeScreenError, NextScene, StopApplication
from asciimatics.scene import Scene
import sqlite3
from datetime import datetime


# Load the shared library using ctypes.
try:
    libvc = ctypes.CDLL("./libvcparser.so")
except Exception as e:
    sys.exit("Could not load libvcparser.so: " + str(e))

# --- Configure shared library functions as needed ---
# For example:
libvc.createCard.argtypes = [ctypes.c_char_p, ctypes.POINTER(c_void_p)]
libvc.createCard.restype = ctypes.c_int

libvc.createMinimalCard.argtypes = [ctypes.POINTER(c_void_p), ctypes.c_char_p]
libvc.createMinimalCard.restype = ctypes.c_int

libvc.editMinimalCard.argtypes = [ctypes.POINTER(c_void_p), ctypes.c_char_p]
libvc.editMinimalCard.restype = ctypes.c_int

libvc.validateCard.argtypes = [c_void_p]
libvc.validateCard.restype = ctypes.c_int

libvc.deleteCard.argtypes = [c_void_p]
libvc.deleteCard.restype = None

libvc.fnToString.argtypes = [c_void_p]  # Expects a Card pointer
libvc.fnToString.restype = ctypes.c_char_p

libvc.bdayToString.argtypes = [c_void_p]
libvc.bdayToString.restype = ctypes.c_char_p

libvc.annToString.argtypes = [c_void_p]
libvc.annToString.restype = ctypes.c_char_p

libvc.numPropsToString.argtypes = [c_void_p]
libvc.numPropsToString.restype = ctypes.c_char_p

# Global variable for passing the selected file.
SELECTED_FILE = ""

class ContactModel:
    def __init__(self):
        # Create a database in RAM.
        self._db = sqlite3.connect(':memory:')
        self._db.row_factory = sqlite3.Row
        self.cursor = self._db.cursor()

        # Create FILE table (SQLite syntax)
        self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS FILE (
                file_id INTEGER PRIMARY KEY AUTOINCREMENT,
                file_name VARCHAR(60) NOT NULL,
                last_modified DATETIME,
                creation_time DATETIME NOT NULL
            )
        """)
        # Create CONTACT table (SQLite syntax)
        self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS CONTACT (
                contact_id INTEGER PRIMARY KEY AUTOINCREMENT,
                name VARCHAR(256) NOT NULL,
                birthday DATETIME,
                anniversary DATETIME,
                file_id INTEGER NOT NULL,
                FOREIGN KEY (file_id) REFERENCES FILE(file_id) ON DELETE CASCADE
            )
        """)
        self._db.commit()

        # Populate from files in the "cards" folder.
        folder = "cards"
        if not os.path.exists(folder):
           # logging.info(f"No '{folder}' directory found.")
           self.scene.add_effect(PopUpDialog(self.screen, f"No '{folder}' directory found.", ["OK"]))

        else:
            for f in os.listdir(folder):
                if not f.endswith(".vcf"):
                    continue
                full_path = os.path.join(folder, f)
                

                # Create a card from the file using createCard.
                card_ptr = c_void_p()
                ret = libvc.createCard(full_path.encode("utf-8"), byref(card_ptr))
                if ret != 0:
                    
                    continue

                # Validate the card.
                ret_val = libvc.validateCard(card_ptr)
                if ret_val != 0:
                    
                    libvc.deleteCard(card_ptr)
                    continue

                # Extract details using shared library functions.
                fn_bytes = libvc.fnToString(card_ptr)
                contact_name = fn_bytes.decode("utf-8") if fn_bytes else ""
                bday_bytes = libvc.bdayToString(card_ptr)
                birthday = bday_bytes.decode("utf-8") if bday_bytes else None
                ann_bytes = libvc.annToString(card_ptr)
                anniversary = ann_bytes.decode("utf-8") if ann_bytes else None

                # Get file modification time.
                mod_time = datetime.fromtimestamp(os.path.getmtime(full_path)).strftime("%Y-%m-%d %H:%M:%S")
                creation_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

                try:
                    self.cursor.execute(
                        "INSERT INTO FILE (file_name, last_modified, creation_time) VALUES (?, ?, ?)",
                        (f, mod_time, creation_time)
                    )
                    file_id = self.cursor.lastrowid
                    self.cursor.execute(
                        "INSERT INTO CONTACT (name, birthday, anniversary, file_id) VALUES (?, ?, ?, ?)",
                        (contact_name, birthday, anniversary, file_id)
                    )
                    self._db.commit()
                 
                except Exception as e:
                    self._db.rollback()
                  

                # Delete the card pointer after use.
                libvc.deleteCard(card_ptr)


    def add(self, contact):
        full_file = contact.get("file", "").strip()
        contact_name = contact.get("contact", "").strip()
        if not full_file or not contact_name:
            logging.error("File name and Contact cannot be empty")
            return

        # Use current time as creation and last_modified timestamps.
        creation_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        last_modified = creation_time

        try:
            self.cursor.execute(
                "INSERT INTO FILE (file_name, last_modified, creation_time) VALUES (?, ?, ?)",
                (full_file, last_modified, creation_time)
            )
            file_id = self.cursor.lastrowid
            self.cursor.execute(
                "INSERT INTO CONTACT (name, file_id) VALUES (?, ?)",
                (contact_name, file_id)
            )
            self._db.commit()
        except Exception as e:
            self._db.rollback()
            logging.error(f"Error adding contact: {e}")



    def update_current_contact(self, details):
        """
        Expects details to be a dictionary with keys:
        "name"      - the new contact name,
        "file_name" - the new file name.
        """
        try:
            # Retrieve the contact_id using file_name.
            row = self.cursor.execute('''
                SELECT contact_id, file_id 
                FROM CONTACT 
                JOIN FILE ON CONTACT.file_id = FILE.file_id
                WHERE FILE.file_name = :file_name
            ''', {"file_name": details["file_name"]}).fetchone()

            if row:
                contact_id = row["contact_id"]
                file_id = row["file_id"]

                # Update the CONTACT table with the new name.
                self.cursor.execute('''
                    UPDATE CONTACT 
                    SET name = :name
                    WHERE contact_id = :contact_id
                ''', {"name": details["name"], "contact_id": contact_id})

                # Update the FILE table with the new file_name.
                self.cursor.execute('''
                    UPDATE FILE 
                    SET file_name = :file_name
                    WHERE file_id = :file_id
                ''', {"file_name": details["file_name"], "file_id": file_id})

                self._db.commit()
            else:
                
                self.scene.add_effect(PopUpDialog(self.screen, f"No record found for file_name: {details['file_name']}", ["OK"]))
        except Exception as e:
            self._db.rollback()
            
            self.scene.add_effect(PopUpDialog(self.screen, f"Error updating contact: {e}", ["OK"]))



class VCardListView(Frame):
    def __init__(self, screen, card_files):
        super(VCardListView, self).__init__(screen, screen.height, screen.width,
                                            has_shadow=True, title="vCard List", name="CardList")
        self.card_files = card_files
        self.list_box = ListBox(
            Widget.FILL_FRAME,
            [(f, f) for f in card_files],
            name="files",
            on_change=self._update_selected_file
        )
        self.selected_file = self.list_box.value

        # HEADER LAYOUT: Buttons at the top.
        header_layout = Layout([25, 25, 25, 25])
        self.add_layout(header_layout)
        header_layout.add_widget(Button("Create", self._create_new), 0)
        header_layout.add_widget(Button("View/Edit", self._view_edit), 1)
        header_layout.add_widget(Button("DB Queries", self._db_queries), 2)
        header_layout.add_widget(Button("Exit", self._quit), 3)
        
        # MAIN LAYOUT: The list box (scrollable).
        main_layout = Layout([100])
        self.add_layout(main_layout)
        main_layout.add_widget(Divider())
        main_layout.add_widget(self.list_box)
        main_layout.add_widget(Divider())
        
        self.fix()
    
    def _update_selected_file(self):
        self.selected_file = self.list_box.value
        # Write debug info to a file:
        

    def _view_edit(self):
        global SELECTED_FILE
        self.selected_file = self.list_box.value
        if self.selected_file:
            SELECTED_FILE = self.selected_file

            raise NextScene("CardDetailsEdit")
            

    
    def _db_queries(self):
        raise NextScene("DBQueries")
    
    def _create_new(self):
        global SELECTED_FILE
        SELECTED_FILE = ""
        raise NextScene("CardDetailsCreate")
    
    def _quit(self):
        raise StopApplication("User requested exit")
    
    def reset(self):
        global SELECTED_FILE
        super(VCardListView, self).reset()
        new_options = [(f, f) for f in os.listdir("cards") if f.endswith(".vcf")]
        self.list_box.options = new_options
        if new_options:
            self.list_box.value = new_options[0][1]
            self.selected_file = self.list_box.value
            SELECTED_FILE = self.selected_file
        else:
            self.list_box.value = None
            self.selected_file = ""
            SELECTED_FILE = ""



class VCardDetailsViewCreate(Frame):
    def __init__(self, screen):
        # Use correct class name in super() call.
        super(VCardDetailsViewCreate, self).__init__(screen, screen.height, screen.width,
                                                       has_shadow=True, title="vCard Details", name="CardDetailsCreate")
        # Use selected file if file_name is empty.
        
        self.card_summary = ""
        # Initialize UI fields as placeholders to prevent attribute errors
        self.file_field = None
        self.name_field = None
        self.bday_field = None
        self.anniv_field = None
        self.other_field = None
        layout = Layout([100])
        self.add_layout(layout)
        # In create mode, allow editing of file name and contact.
        self.file_field = Text("File name:", "file")
        self.name_field = Text("Contact:", "contact")
        # These fields are read-only.
        self.bday_field = Text("Birthday:", "bday", readonly=True)
        self.anniv_field = Text("Anniversary:", "anniv", readonly=True)
        self.other_field = Text("Other properties:", "other", readonly=True)
        layout.add_widget(self.file_field)
        layout.add_widget(self.name_field)
        # Do not show date fields in create mode.
        layout.add_widget(Divider())
        layout2 = Layout([50, 50])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)
        self.fix()
        self.load_details()
    
    def load_details(self):
        # For creating a new card, initialize fields as empty.
        self.data["file"] = ""
        self.data["contact"] = ""
        self.data["bday"] = ""
        self.data["anniv"] = ""
        self.data["other"] = ""
        self.card_summary = "Creating new card"
    
    def _ok(self):
        self.save()  # Update self.data with the latest widget values.
        new_file = self.data.get("file", "").strip()
        contact = self.data.get("contact", "").strip()
        if not new_file or not contact:
            self.scene.add_effect(PopUpDialog(self.screen, "File name and Contact cannot be empty", ["OK"]))
            return
        full_path = os.path.join("cards", new_file)
        if os.path.exists(full_path):
            self.scene.add_effect(PopUpDialog(self.screen, "File already exists", ["OK"]))
            return

        # Call createMinimalCard: signature: createMinimalCard(Card** card, char* fn)
        card_ptr = c_void_p()
        ret = libvc.createMinimalCard(byref(card_ptr), contact.encode("utf-8"))
        if ret != 0:
            self.scene.add_effect(PopUpDialog(self.screen, f"Error creating card: {ret}", ["OK"]))
            return
        else:
            ret2 = libvc.validateCard(card_ptr)
            if ret2 != 0:
                self.scene.add_effect(PopUpDialog(self.screen, "Card validation failed", ["OK"]))
                libvc.deleteCard(card_ptr)
                return
            else:
                # Call writeCard: signature: writeCard(const char* fileName, const Card* obj);
                ret3 = libvc.writeCard(full_path.encode("utf-8"), card_ptr)
                if ret3 != 0:
                    self.scene.add_effect(PopUpDialog(self.screen, "Writing to file failed", ["OK"]))
                    libvc.deleteCard(card_ptr)
                    return
                self.scene.add_effect(PopUpDialog(self.screen, "Card saved successfully", ["OK"]))
            libvc.deleteCard(card_ptr)
        contacts.add({"file": new_file, "contact": contact})

        raise NextScene("CardList")

    
    def _cancel(self):
        raise NextScene("CardList")
    
    def reset(self):
        super(VCardDetailsViewCreate, self).reset()
        self.load_details()


class VCardDetailsViewEdit(Frame):
    def __init__(self, screen):
        # Initialize the frame.
        super(VCardDetailsViewEdit, self).__init__(screen, screen.height, screen.width,
                                                   has_shadow=True, title="Edit vCard", name="CardDetailsEdit")
        self.card_summary = ""
        
        # First layout: prompt for file name.
        layout_prompt = Layout([100])
        self.add_layout(layout_prompt)
        self.file_field = Text("Enter file name to edit:", "file")
        layout_prompt.add_widget(self.file_field)
        layout_prompt.add_widget(Divider())
        
        # Second layout: a button to load details.
        layout_button = Layout([50, 50])
        self.add_layout(layout_button)
        layout_button.add_widget(Button("Load", self._load), 0)
        layout_button.add_widget(Button("Cancel", self._cancel), 1)
        
        self.fix()
    
    def _load(self):
        # Save the prompt field to update self.data.
        self.save()
        file_name = self.data.get("file", "").strip()
        
        if not file_name or not os.path.exists(os.path.join("cards", file_name)):
            self.scene.add_effect(PopUpDialog(self.screen, "Please enter a file name that is in the list.", ["OK"]))
            return
        
        # Clear previous layouts and widgets.
        
        
        # Create a new layout for displaying the card details.
        layout_details = Layout([100])
        self.add_layout(layout_details)
        
        # In edit mode, file name is read-only.
        
        self.file_field = Text("File name:", "file", readonly=True)
        self.name_field = Text("Contact:", "contact")
        self.bday_field = Text("Birthday:", "bday", readonly=True)
        self.anniv_field = Text("Anniversary:", "anniv", readonly=True)
        self.other_field = Text("Other properties:", "other", readonly=True)
        layout_details.add_widget(self.file_field)
        layout_details.add_widget(self.name_field)
        layout_details.add_widget(self.bday_field)
        layout_details.add_widget(self.anniv_field)
        layout_details.add_widget(self.other_field)
        layout_details.add_widget(Divider())
        
        # Create a new layout for OK/Cancel buttons.
        layout_buttons = Layout([50, 50])
        self.add_layout(layout_buttons)
        layout_buttons.add_widget(Button("OK", self._ok), 0)
        layout_buttons.add_widget(Button("Cancel", self._cancel), 1)
        
        # Save the entered file name in self.data.
        self.data["file"] = file_name
        
        # Load details from the file.
        self.load_details(file_name)
        # Update UI fields correctly after loading
        self.file_field.value = self.data["file"]
        self.name_field.value = self.data["contact"]
        self.bday_field.value = self.data["bday"]
        self.anniv_field.value = self.data["anniv"]
        self.other_field.value = self.data["other"]

        self.fix()
    
    def load_details(self, file_name):
        with open("debug_log.txt", "a") as f:
            print(f"Debugging: {file_name}", file=f)

        self.data = {"file": file_name, "contact": "", "bday": "", "anniv": "", "other": ""}
        
        full_path = os.path.join("cards", file_name)
        card_ptr = c_void_p()
        ret = libvc.createCard(full_path.encode("utf-8"), byref(card_ptr))
        
        if ret != 0:
            self.card_summary = f"Error creating card: {ret}"
            self.data["contact"] = "Error"
            self.data["bday"] = "Error"
            self.data["anniv"] = "Error"
            self.data["other"] = "Error"
            return
        
        # Decode and assign property values.
        fn_ptr = libvc.fnToString(card_ptr)
        bday_ptr = libvc.bdayToString(card_ptr)
        ann_ptr = libvc.annToString(card_ptr)
        numProps_ptr = libvc.numPropsToString(card_ptr)
        
        fnStr = fn_ptr.decode("utf-8") if fn_ptr and fn_ptr != b'' else "No contact"
        bdayStr = bday_ptr.decode("utf-8") if bday_ptr and bday_ptr != b'' else "No birthday"
        annStr = ann_ptr.decode("utf-8") if ann_ptr and ann_ptr != b'' else "No anniversary"
        numPropsStr = numProps_ptr.decode("utf-8") if numProps_ptr and numProps_ptr != b'' else "No other properties"
        
        # Update the data.
        self.data["file"] = file_name
        self.data["contact"] = fnStr
        self.data["bday"] = bdayStr
        self.data["anniv"] = annStr
        self.data["other"] = numPropsStr
        
        self.card_summary = "Card loaded successfully"

        # Debug logs.
        with open("debug_log.txt", "a") as f:
            print(f"Debugging: {self.data['contact']}", file=f)
            print(f"Debugging: {self.data['bday']}", file=f)
            print(f"Debugging: {self.data['anniv']}", file=f)
            print(f"Debugging: {self.data['other']}", file=f)

        libvc.deleteCard(card_ptr)
    
    def _ok(self):
        self.save()  # Update self.data with the latest widget values.
        
        file_name = self.data.get("file", "").strip()
        contact = self.data.get("contact", "").strip()

        if not contact:
            self.scene.add_effect(PopUpDialog(self.screen, "Contact cannot be empty", ["OK"]))
            return

        full_path = os.path.join("cards", file_name)
        card_ptr = c_void_p()  # Use a single pointer for card operations

        # Create a new card
        ret = libvc.createCard(full_path.encode("utf-8"), byref(card_ptr))
        if ret != 0 or not card_ptr:
            self.scene.add_effect(PopUpDialog(self.screen, f"Error creating card: {ret}", ["OK"]))
            # Cleanup
            libvc.deleteCard(card_ptr)
            raise NextScene("CardList")
            return

        # Set more card properties (such as Name) before editing if there's another valid function
        ret2 = libvc.editMinimalCard(card_ptr, contact.encode("utf-8"))
        if ret2 != 0:
            self.scene.add_effect(PopUpDialog(self.screen, "Card editing failed", ["OK"]))
            libvc.deleteCard(card_ptr)
            return

        # Validate the card to ensure correctness
        ret3 = libvc.validateCard(card_ptr)
        if ret3 != 0:
            # Adding more detailed debug info for validation failure
            self.scene.add_effect(PopUpDialog(self.screen, f"Card validation failed. Error code: {ret3}", ["OK"]))
            libvc.deleteCard(card_ptr)
            return

        # Write the card to the file
        ret4 = libvc.writeCard(full_path.encode("utf-8"), card_ptr)
        if ret4 != 0:
            self.scene.add_effect(PopUpDialog(self.screen, "Writing to file failed", ["OK"]))
            libvc.deleteCard(card_ptr)
            return

        # Successfully saved the card
        contacts.update_current_contact({"name": contact, "file_name": file_name})
        self.scene.add_effect(PopUpDialog(self.screen, "Card saved successfully", ["OK"]))
        
        # Cleanup
        libvc.deleteCard(card_ptr)

        # Transition to the "CardList" scene after successful save
        raise NextScene("CardList")

    
    def _cancel(self):
        raise NextScene("CardList")
    

    def reset(self):
        super(VCardDetailsViewEdit, self).reset()
        self.data = {
            "file": "",
            "contact": "",
            "bday": "",
            "anniv": "",
            "other": ""
        }

        # Check if fields exist before assigning values
        if hasattr(self, "name_field"):
            self.name_field.value = ""
        if hasattr(self, "bday_field"):
            self.bday_field.value = ""
        if hasattr(self, "anniv_field"):
            self.anniv_field.value = ""
        if hasattr(self, "other_field"):
            self.other_field.value = ""

class DBQueryView(Frame):
    def __init__(self, screen, db_model):
        super(DBQueryView, self).__init__(screen,
                                          screen.height * 2 // 3,
                                          screen.width * 2 // 3,
                                          name="DBQueries")
        self.db_model = db_model
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        self.query_result = TextBox(Widget.FILL_FRAME, "Query Results", "results", as_string=True, readonly=True)
        layout.add_widget(self.query_result)
        layout2 = Layout([1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Display All Contacts", self._display_all), 0)
        layout2.add_widget(Button("Find June Birthdays", self._find_june), 1)
        layout2.add_widget(Button("Cancel", self._cancel), 2)
        self.fix()

    def _display_all(self):
        q = """
            SELECT c.contact_id, c.name, c.birthday, c.anniversary, f.file_name
            FROM CONTACT c JOIN FILE f ON c.file_id = f.file_id
            ORDER BY c.name
        """
        self.db_model.cursor.execute(q)
        rows = self.db_model.cursor.fetchall()
        result = "\n".join([", ".join(map(str, row)) for row in rows])
        self.data["results"] = result
        if self.data["results"]:
            self.query_result.value = self.data["results"]
        else:
            self.query_result.value = "Nothing to return."
        self.save()
        self.fix()

    def _find_june(self):
        q = """
            SELECT name, birthday FROM CONTACT
            WHERE strftime('%m', birthday) = '06'
            ORDER BY julianday('now') - julianday(birthday)
        """
        self.db_model.cursor.execute(q)
        rows = self.db_model.cursor.fetchall()
        result = "\n".join([", ".join(map(str, row)) for row in rows])
        self.data["results"] = result
        if self.data["results"]:
            self.query_result.value = self.data["results"]
        else:
            self.query_result.value = "Nothing to return."

        self.save()
        self.fix()

    def _cancel(self):
        raise NextScene("CardList")



def get_scenes(screen):
    return [
        Scene([VCardListView(screen, os.listdir("cards"))], -1, name="CardList"),
        Scene([VCardDetailsViewCreate(screen)], -1, name="CardDetailsCreate"),
        Scene([VCardDetailsViewEdit(screen)], -1, name="CardDetailsEdit"),
        Scene([DBQueryView(screen, contacts)], -1, name="DBQueries")
    ]

def demo_ui(screen):
    while True:
        try:
            screen.play(get_scenes(screen), stop_on_resize=True)
            break
        except ResizeScreenError:
            pass
contacts = ContactModel()
def main():
    while True:
        try:
            Screen.wrapper(demo_ui)
            sys.exit(0)
        except ResizeScreenError:
            pass

if __name__ == "__main__":
    main()
