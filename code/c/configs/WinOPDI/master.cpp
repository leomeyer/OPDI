//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011 Leo Meyer (leo@leomeyer.de)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "stdafx.h"

#include "opdi_constants.h"
#include "master.h"
#include "hef_uri_syntax.h"

const wchar_t *read_line (wchar_t *buf, size_t length, FILE *f)
  /**** Read at most 'length'-1 characters from the file 'f' into
        'buf' and zero-terminate this character sequence. If the
        line contains more characters, discard the rest.
   */
{
  const wchar_t *p;

  if (p = fgetws (buf, length, f)) {
    size_t last = wcslen (buf) - 1;

    if (buf[last] == '\n') {
      /**** Discard the trailing newline */
      buf[last] = '\0';
    } else {
      /**** There's no newline in the buffer, therefore there must be
            more characters on that line: discard them!
       */
      fscanf (f, "%*[^\n]");
      /**** And also discard the newline... */
      (void) fgetc (f);
    } /* end if */
  } /* end if */
  return p;
} /* end read_line */


void show_help() {
	printf("Interactive OPDI master commands:\n");
	printf("? - show help\n");
	printf("quit - exit\n");
	printf("connect <address> - connect to the specified slave\n");
	printf("\n");
	printf("Get started: Run slave and try 'connect opdi_tcp://localhost:13110'\n");
}

void connect_slave(const wchar_t *address) {
	const wchar_t *part;
	// convert address to std::string
#define ADDR_MAXLEN 512
	char ch[ADDR_MAXLEN];
    char DefChar = ' ';
    WideCharToMultiByte(CP_ACP, 0, address, -1, ch, ADDR_MAXLEN, &DefChar, NULL);
    std::string addr(ch);
	
	// parse address
	hef::HfURISyntax uri(addr);
	if (strcmp(uri.getScheme().c_str(), "opdi_tcp") == 0) {
		wprintf(L"TCP address: %s\n", address);
		printf("host: %s, port: %d\n", uri.getHost().c_str(), (int)uri.getPort());
		printf("path: %s\n", uri.getPath().c_str());
		printf("parameters: %s\n", uri.getQuery().c_str());
	} else {
		wprintf(L"address not recognized\n");
		return;
	}


}

int start_master() {

	#define BUFFER_LENGTH   255
	#define PROMPT			"$ "

	wchar_t cmd[BUFFER_LENGTH];
	const wchar_t *part;

	printf("Interactive OPDI master started. Type '?' for help.\n");

	printf(PROMPT);
	fflush(stdout);

	while (read_line(cmd, BUFFER_LENGTH, stdin)) {

		// tokenize the input
		part = wcstok(cmd, _T(" "));
		// evaluate command
        if (_tcscmp(part, _T("?")) == 0) {
			show_help();
		} else
        if (_tcsicmp(part, _T("quit")) == 0) {
			break;
		} else
        if (_tcsicmp(part, _T("connect")) == 0) {
			// expect second token: slave address
			part = wcstok(NULL, _T(" "));
			if (part == NULL) {
				printf("error: slave address expected\n");
			} else {
				connect_slave(part);
			}
		} else {
			printf("command unknown\n");
		}
		printf(PROMPT);
		fflush(stdout);
	}

	return 0;
}