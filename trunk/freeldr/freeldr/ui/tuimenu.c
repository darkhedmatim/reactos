/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#include <freeldr.h>
#include <rtl.h>
#include <ui.h>
#include "tui.h"
#include "keycodes.h"
#include <options.h>
#include <mm.h>


BOOL TuiDisplayMenu(PUCHAR MenuItemList[], U32 MenuItemCount, U32 DefaultMenuItem, S32 MenuTimeOut, U32* SelectedMenuItem)
{
	TUI_MENU_INFO	MenuInformation;
	U32				CurrentClockSecond;

	//
	// The first thing we need to check is the timeout
	// If it's zero then don't bother with anything,
	// just return the default item
	//
	if (MenuTimeOut == 0)
	{
		if (SelectedMenuItem != NULL)
		{
			*SelectedMenuItem = DefaultMenuItem;
		}

		return TRUE;
	}

	//
	// Setup the MENU_INFO structure
	//
	MenuInformation.MenuItemList = MenuItemList;
	MenuInformation.MenuItemCount = MenuItemCount;
	MenuInformation.MenuTimeRemaining = MenuTimeOut;
	MenuInformation.SelectedMenuItem = DefaultMenuItem;

	//
	// Calculate the size of the menu box
	//
	TuiCalcMenuBoxSize(&MenuInformation);

	//
	// Draw the menu
	//
	TuiDrawMenu(&MenuInformation);

	//
	// Get the current second of time
	//
	CurrentClockSecond = getsecond();

	//
	// Process keys
	//
	while (1)
	{
		//
		// Process key presses
		//
		if (TuiProcessMenuKeyboardEvent(&MenuInformation) == KEY_ENTER)
		{
			//
			// If they pressed enter then exit this loop
			//
			break;
		}

		//
		// Update the date & time
		//
		UiUpdateDateTime();

		if (MenuInformation.MenuTimeRemaining > 0)
		{
			if (getsecond() != CurrentClockSecond)
			{
				//
				// Update the time information
				//
				CurrentClockSecond = getsecond();
				MenuInformation.MenuTimeRemaining--;

				//
				// Update the menu
				//
				TuiDrawMenuBox(&MenuInformation);
			}
		}
		else if (MenuInformation.MenuTimeRemaining == 0)
		{
			//
			// A time out occurred, exit this loop and return default OS
			//
			break;
		}
	}

	//
	// Update the selected menu item information
	//
	if (SelectedMenuItem != NULL)
	{
		*SelectedMenuItem = MenuInformation.SelectedMenuItem;
	}
	
	return TRUE;
}

VOID TuiCalcMenuBoxSize(PTUI_MENU_INFO MenuInfo)
{
	U32		Idx;
	U32		Width;
	U32		Height;
	U32		Length;

	//
	// Height is the menu item count plus 2 (top border & bottom border)
	//
	Height = MenuInfo->MenuItemCount + 2;
	Height -= 1; // Height is zero-based

	//
	// Find the length of the longest string in the menu
	//
	Width = 0;
	for(Idx=0; Idx<MenuInfo->MenuItemCount; Idx++)
	{
		Length = strlen(MenuInfo->MenuItemList[Idx]);

		if (Length > Width)
		{
			Width = Length;
		}
	}

	//
	// Allow room for left & right borders, plus 8 spaces on each side
	//
	Width += 18;

	//
	// Calculate the menu box area
	//
	MenuInfo->Left = (UiScreenWidth - Width) / 2;
	MenuInfo->Right = (MenuInfo->Left) + Width;
	MenuInfo->Top = (( (UiScreenHeight - TUI_TITLE_BOX_CHAR_HEIGHT) - Height) / 2 + 1) + (TUI_TITLE_BOX_CHAR_HEIGHT / 2);
	MenuInfo->Bottom = (MenuInfo->Top) + Height;
}

VOID TuiDrawMenu(PTUI_MENU_INFO MenuInfo)
{
	U32		Idx;

	//
	// Draw the menu box
	//
	TuiDrawMenuBox(MenuInfo);

	//
	// Draw each line of the menu
	//
	for (Idx=0; Idx<MenuInfo->MenuItemCount; Idx++)
	{
		TuiDrawMenuItem(MenuInfo, Idx);
	}
}

VOID TuiDrawMenuBox(PTUI_MENU_INFO MenuInfo)
{
	UCHAR	MenuLineText[80];
	UCHAR	TempString[80];

	//
	// Update the status bar
	//
	UiDrawStatusText("Use \x18\x19 to select, ENTER to boot.");

	//
	// Draw the menu box
	//
	UiDrawBox(MenuInfo->Left,
		MenuInfo->Top,
		MenuInfo->Right,
		MenuInfo->Bottom,
		D_VERT,
		D_HORZ,
		FALSE,		// Filled
		TRUE,		// Shadow
		ATTR(UiMenuFgColor, UiMenuBgColor));

	//
	// If there is a timeout draw the time remaining
	//
	if (MenuInfo->MenuTimeRemaining >= 0)
	{
		strcpy(MenuLineText, "[ Time Remaining: ");
		itoa(MenuInfo->MenuTimeRemaining, TempString, 10);
		strcat(MenuLineText, TempString);
		strcat(MenuLineText, " ]");

		UiDrawText(MenuInfo->Right - strlen(MenuLineText) - 1,
			MenuInfo->Bottom,
			MenuLineText,
			ATTR(UiMenuFgColor, UiMenuBgColor));
	}
}

VOID TuiDrawMenuItem(PTUI_MENU_INFO MenuInfo, U32 MenuItemNumber)
{
	U32		Idx;
	UCHAR	MenuLineText[80];
	U32		SpaceTotal;
	U32		SpaceLeft;
	U32		SpaceRight;

	//
	// We will want the string centered so calculate
	// how many spaces will be to the left and right
	//
	SpaceTotal = (MenuInfo->Right - MenuInfo->Left - 2) - strlen(MenuInfo->MenuItemList[MenuItemNumber]);
	SpaceLeft = (SpaceTotal / 2) + 1;
	SpaceRight = (SpaceTotal - SpaceLeft) + 1;

	//
	// Insert the spaces on the left
	//
	for (Idx=0; Idx<SpaceLeft; Idx++)
	{
		MenuLineText[Idx] = ' ';
	}
	MenuLineText[Idx] = '\0';

	//
	// Now append the text string
	//
	strcat(MenuLineText, MenuInfo->MenuItemList[MenuItemNumber]);

	//
	// Now append the spaces on the right
	//
	for (Idx=0; Idx<SpaceRight; Idx++)
	{
		strcat(MenuLineText, " ");
	}

	//
	// If this is the selected menu item then draw it as selected
	// otherwise just draw it using the normal colors
	//
	if (MenuItemNumber == MenuInfo->SelectedMenuItem)
	{
		UiDrawText(MenuInfo->Left + 1,
			MenuInfo->Top + 1 + MenuItemNumber,
			MenuLineText,
			ATTR(UiSelectedTextColor, UiSelectedTextBgColor));
	}
	else
	{
		UiDrawText(MenuInfo->Left + 1,
			MenuInfo->Top + 1 + MenuItemNumber,
			MenuLineText,
			ATTR(UiTextColor, UiMenuBgColor));
	}
}

U32 TuiProcessMenuKeyboardEvent(PTUI_MENU_INFO MenuInfo)
{
	U32		KeyEvent = 0;

	//
	// Check for a keypress
	//
	if (kbhit())
	{
		//
		// Cancel the timeout
		//
		if (MenuInfo->MenuTimeRemaining != -1)
		{
			MenuInfo->MenuTimeRemaining = -1;
			TuiDrawMenuBox(MenuInfo);
		}

		//
		// Get the key
		//
		KeyEvent = getch();

		//
		// Is it extended?
		//
		if (KeyEvent == 0)
			KeyEvent = getch(); // Yes - so get the extended key

		//
		// Process the key
		//
		switch (KeyEvent)
		{
		case KEY_UP:
			
			if (MenuInfo->SelectedMenuItem > 0)
			{
				MenuInfo->SelectedMenuItem--;

				//
				// Update the menu
				//
				TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem + 1);	// Deselect previous item
				TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);		// Select new item
			}

			break;

		case KEY_DOWN:

			if (MenuInfo->SelectedMenuItem < (MenuInfo->MenuItemCount - 1))
			{
				MenuInfo->SelectedMenuItem++;

				//
				// Update the menu
				//
				TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem - 1);	// Deselect previous item
				TuiDrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);		// Select new item
			}

			break;
		}
	}

	return KeyEvent;
}
