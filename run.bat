@echo off
rem Start the command menu from any folder. Double-click this file, or run it in a terminal.
cd /d "%~dp0"
uv run python tools\menu.py
if errorlevel 1 pause
