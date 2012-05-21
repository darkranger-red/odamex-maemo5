// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_misc.cpp 3174 2012-05-11 01:03:43Z mike $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//		Default Config File.
//		1/5/12: JSON File Functions
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "c_cvars.h"
#include "c_dispatch.h"
#include "doomdef.h"
#include "doomtype.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "i_system.h"
#include "version.h"
#include "sv_main.h"
#include "sv_master.h"


// Used to identify the version of the game that saved
// a config file to compensate for new features that get
// put into newer configfiles.
static CVAR (configver, CONFIGVERSIONSTR, "", CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// [RH] Get configfile path.
// This file contains commands to set all
// archived cvars, bind commands to keys,
// and set other general game information.
std::string GetConfigPath (void)
{
	const char *p = Args.CheckValue ("-config");

	if(p)
		return p;

	return I_GetUserFileName ("odasrv.cfg");
}

//
// M_SaveDefaults
//

// [RH] Don't write a config file if M_LoadDefaults hasn't been called.
bool DefaultsLoaded;

void STACK_ARGS M_SaveDefaults (void)
{
	FILE *f;

	if (!DefaultsLoaded)
		return;
	
	// [ML] Always save to the default.
	std::string configfile = I_GetUserFileName ("odasrv.cfg");

	// Make sure the user hasn't changed configver
	configver.Set(CONFIGVERSIONSTR);

	if ( (f = fopen (configfile.c_str(), "w")) )
	{
		fprintf (f, "// Generated by Odasrv " DOTVERSIONSTR "\n\n");

		// Archive all cvars marked as CVAR_ARCHIVE
		fprintf (f, "// --- Console variables ---\n\n");
		cvar_t::C_ArchiveCVars (f);

		// Archive all active key bindings
		//fprintf (f, "// --- Key Bindings ---\n\n");
		//C_ArchiveBindings (f);

		// Archive all aliases
		fprintf (f, "\n// --- Aliases ---\n\n");
		DConsoleAlias::C_ArchiveAliases (f);

		fclose (f);
	}
}


//
// M_LoadDefaults
//
extern int cvar_defflags;
EXTERN_CVAR (dimamount)

void M_LoadDefaults (void)
{
	// Set default key bindings. These will be overridden
	// by the bindings in the config file if it exists.
	//AddCommandString (DefBindings);

	std::string cmd = "exec \"";
	cmd += GetConfigPath();
	cmd += "\"";

	cvar_defflags = CVAR_ARCHIVE;
	AddCommandString (cmd.c_str());
	cvar_defflags = 0;

	if (configver < 118.0f)
	{
		AddCommandString ("alias idclev map ");		
		AddCommandString ("alias changemap map ");
		AddCommandString ("alias ? help ");	
	}

	DefaultsLoaded = true;
	atterm (M_SaveDefaults);
}

// JSON Utility Functions (based on those from EECS)

// Reads a file in JSON format
bool M_ReadJSON(Json::Value &json, const char *filename) {
	byte *buffer = NULL;
	std::string data;
	Json::Reader reader;

	if (!(M_FileExists(filename))) {
		return false;
	}

	if (!M_ReadFile(filename, &buffer)) {
		return false;
	}
	data = (char *)buffer;

	if (!reader.parse(data, json)) {
		Printf(PRINT_HIGH,"M_ReadJSONFromFile: Error parsing JSON: %s.\n",
				reader.getFormattedErrorMessages().c_str());
		return false;
	}

	return true;
}

// Writes a file in JSON format.  Third param is true if the output
// should be pretty-printed.
bool M_WriteJSON(const char *filename, Json::Value &value, bool styled) {
	std::ofstream out_file;
	Json::FastWriter fast_writer;
	Json::StyledWriter styled_writer;

	out_file.open(filename);

	if (styled) {
		out_file << styled_writer.write(value);
	} else {
		out_file << fast_writer.write(value);
	}

	out_file.close();

	if (out_file.fail()) {
		return false;
	}
	return true;
}

VERSION_CONTROL (m_misc_cpp, "$Id: m_misc.cpp 3174 2012-05-11 01:03:43Z mike $")
