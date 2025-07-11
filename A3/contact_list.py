#!/usr/bin/env python3
"""
A refactored vCard manager using Asciimatics with multiple views.
This version uses an in‑memory SQLite database instead of a remote MySQL connection.
"""

import ctypes
from ctypes import c_void_p, byref
import sys
import os
import time
import sqlite3
from datetime import datetime

from asciimatics.widgets import Frame, ListBox, Layout, Divider, Text, TextBox, Button, Widget, PopUpDialog
from asciimatics.scene import Scene
from asciimatics.screen import Screen
from asciimatics.exceptions import ResizeScreenError, NextScene, StopApplication

# Load the shared library using ctypes.
try:
    libvc = ctypes.CDLL("./libvcparser.so")
except OSError as e:
    print(f"Could not load libvcparser.so: {e}")
    sys.exit(1)

# --- Configure shared library functions as needed ---
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

libvc.fnToString.argtypes = [c_void_p]
libvc.fnToString.restype = ctypes.c_char_p

libvc.bdayToString.argtypes = [c_void_p]
libvc.bdayToString.restype = ctypes.c_char_p

libvc.annToString.argtypes = [c_void_p]
libvc.annToString.restype = ctypes.c_char_p

libvc.numPropsToString.argtypes = [c_void_p]
libvc.numPropsToString.restype = ctypes.c_char_p

# =============================================================================
# Database Model using SQLite in memory.
# =============================================================================
class DatabaseModel:
    """
    Manages the DB connection and operations for vCard files.
    Creates two tables: FILE and CONTACT.
    """
    def __init__(self, connection):
        self.conn = connection
        self.conn.row_factory = sqlite3.Row
        self.cursor = self.conn.cursor()
        self._create_tables()

    def _create_tables(self):
        # Create FILE table
        self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS FILE (
                file_id INTEGER PRIMARY KEY AUTOINCREMENT,
                file_name VARCHAR(60) NOT NULL,
                last_modified DATETIME,
                creation_time DATETIME NOT NULL
            )
        """)
        # Create CONTACT table
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
        self.conn.commit()

    def insert_file_record(self, file_name, mod_time, creation_time):
        q = """INSERT INTO FILE (file_name, last_modified, creation_time)
               VALUES (?, ?, ?)"""
        self.cursor.execute(q, (file_name, mod_time, creation_time))
        self.conn.commit()
        return self.cursor.lastrowid

    def insert_contact_record(self, name, birthday, anniversary, file_id):
        q = """INSERT INTO CONTACT (name, birthday, anniversary, file_id)
               VALUES (?, ?, ?, ?)"""
        self.cursor.execute(q, (name, birthday, anniversary, file_id))
        self.conn.commit()

# =============================================================================
# vCard List View (Main View)
# =============================================================================
class VCardListView(Frame):
    def __init__(self, screen, db_model):
        super(VCardListView, self).__init__(screen,
                                            screen.height * 2 // 3,
                                            screen.width * 2 // 3,
                                            name="vCard List")
        self.db_model = db_model
        self.cards_folder = os.path.join(os.getcwd(), "cards")
        self.vcard_files = self.scan_vcard_files()
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        self._listbox = ListBox(Widget.FILL_FRAME,
                                self.vcard_files,
                                name="vcards",
                                add_scroll_bar=True,
                                on_change=self._on_pick,
                                on_select=self._edit)
        layout.add_widget(self._listbox)
        layout.add_widget(Divider())
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Edit", self._edit), 0)
        layout2.add_widget(Button("Create", self._create), 1)
        layout2.add_widget(Button("DB Query", self._db_query), 2)
        layout2.add_widget(Button("Exit", self._quit), 3)
        self.fix()
        self._on_pick()

    def scan_vcard_files(self):
        files = []
        if os.path.exists(self.cards_folder):
            for f in os.listdir(self.cards_folder):
                if f.endswith(".vcf"):
                    # Here, integrate your Ctypes calls to create and validate vCards.
                    files.append((f, f))
        return files

    def _on_pick(self):
        # Optionally enable/disable buttons based on selection.
        pass

    def _edit(self):
        self.save()
        self.selected_file = self.data["vcards"]
        raise NextScene("vCard Details")

    def _create(self):
        self.selected_file = None
        raise NextScene("vCard Details")

    def _db_query(self):
        raise NextScene("DB Query")

    @staticmethod
    def _quit():
        raise StopApplication("User requested exit")

# =============================================================================
# vCard Details View
# =============================================================================
class VCardDetailsView(Frame):
    def __init__(self, screen, db_model):
        super(VCardDetailsView, self).__init__(screen,
                                               screen.height * 2 // 3,
                                               screen.width * 2 // 3,
                                               name="vCard Details")
        self.db_model = db_model
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        self.filename_field = Text("File name:", "filename")
        self.contact_field = Text("Contact Name:", "contact")
        self.birthday_field = Text("Birthday:", "birthday")
        self.anniversary_field = Text("Anniversary:", "anniversary")
        self.other_field = Text("Other props:", "other", readonly=True)
        layout.add_widget(self.filename_field)
        layout.add_widget(self.contact_field)
        layout.add_widget(self.birthday_field)
        layout.add_widget(self.anniversary_field)
        layout.add_widget(self.other_field)
        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)
        self.fix()

    def reset(self):
        super(VCardDetailsView, self).reset()
        self.data = {"filename": "", "contact": "", "birthday": "", "anniversary": "", "other": "0"}

    def _ok(self):
        self.save()
        new_file = self.data.get("filename", "").strip()
        contact = self.data.get("contact", "").strip()
        if not new_file or not contact:
            self.scene.add_effect(PopUpDialog(self.screen, "File name and Contact cannot be empty", ["OK"]))
            return
        full_path = os.path.join("cards", new_file)
        if os.path.exists(full_path):
            self.scene.add_effect(PopUpDialog(self.screen, "File already exists", ["OK"]))
            return

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
                # Assuming writeCard now takes (filename, card_ptr)
                ret3 = libvc.writeCard(full_path.encode("utf-8"), card_ptr)
                if ret3 != 0:
                    self.scene.add_effect(PopUpDialog(self.screen, "Writing to file failed", ["OK"]))
                    libvc.deleteCard(card_ptr)
                    return
                self.scene.add_effect(PopUpDialog(self.screen, "Card saved successfully", ["OK"]))
            libvc.deleteCard(card_ptr)
        raise NextScene("vCard List")

    def _cancel(self):
        raise NextScene("vCard List")

# =============================================================================
# DB Query View
# =============================================================================
class DBQueryView(Frame):
    def __init__(self, screen, db_model):
        super(DBQueryView, self).__init__(screen,
                                          screen.height * 2 // 3,
                                          screen.width * 2 // 3,
                                          name="DB Query")
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
        self.save()
        self.fix()

    def _cancel(self):
        raise NextScene("vCard List")

# =============================================================================
# Main Demo Function
# =============================================================================
def demo(screen, scene):
    print("Entering demo function...")  # Debug message (visible if run in a terminal)
    # Create a SQLite in-memory database.
    conn = sqlite3.connect(':memory:')
    conn.row_factory = sqlite3.Row
    db_model = DatabaseModel(conn)
    scenes = [
        Scene([VCardListView(screen, db_model)], -1, name="vCard List"),
        Scene([VCardDetailsView(screen, db_model)], -1, name="vCard Details"),
        Scene([DBQueryView(screen, db_model)], -1, name="DB Query")
    ]
    screen.play(scenes, stop_on_resize=True, start_scene=scene, allow_int=True)


# =============================================================================
# Main loop
# =============================================================================
last_scene = None
while True:
    try:
        Screen.wrapper(demo, catch_interrupt=True, arguments=[last_scene])
        sys.exit(0)
    except ResizeScreenError as e:
        last_scene = e.scene
