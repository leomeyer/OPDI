//    This file is part of an OPDI reference implementation.
//    see: Open Protocol for Device Interaction
//
//    Copyright (C) 2011-2014 Leo Meyer (leo@leomeyer.de)
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
    
// Protocol constant definitions

// Not for inclusion by configs!

#define OPDI_PARTS_SEPARATOR			':'
#define OPDI_MULTIMESSAGE_SEPARATOR		'\r'

#define OPDI_Handshake 					"OPDI"
#define OPDI_Handshake_version 			"0.1"

#define OPDI_Basic_protocol_magic 		"BP"
#define OPDI_Extended_protocol_magic	"EP"

// control channel message identifiers
#define OPDI_Error 						"Err"
#define OPDI_Disconnect 				"Dis"
#define OPDI_Refresh 					"Ref"
#define OPDI_Reconfigure 				"Reconf"
#define OPDI_Debug 						"Debug"

#define OPDI_Agreement 					"OK"
#define OPDI_Disagreement 				"NOK"

#define OPDI_Auth 						"Auth"

// protocol message identifiers
#define OPDI_getDeviceCaps  			"gDC"
#define OPDI_getPortInfo  				"gPI"

#define OPDI_analogPort  				"AP"
#define OPDI_analogPortState  			"AS"
#define OPDI_getAnalogPortState  		"gAS"
#define OPDI_setAnalogPortValue  		"sAV"
#define OPDI_setAnalogPortMode  		"sAM"
#define OPDI_setAnalogPortResolution	"sAR"
#define OPDI_setAnalogPortReference		"sARF"

#define OPDI_digitalPort  				"DP"
#define OPDI_digitalPortState  			"DS"
#define OPDI_getDigitalPortState  		"gDS"
#define OPDI_setDigitalPortLine  		"sDL"
#define OPDI_setDigitalPortMode  		"sDM"

#define OPDI_selectPort  				"SLP"
#define OPDI_getSelectPortLabel  		"gSL"
#define OPDI_selectPortLabel  			"SL"
#define OPDI_selectPortState  			"SS"
#define OPDI_getSelectPortState  		"gSS"
#define OPDI_setSelectPortPosition		"sSP"

#define OPDI_dialPort  					"DL"
#define OPDI_dialPortState  			"DLS"
#define OPDI_getDialPortState  			"gDLS"
#define OPDI_setDialPortPosition  		"sDLP"

#define OPDI_streamingPort  			"SP"
#define OPDI_bindStreamingPort  		"bSP"
#define OPDI_unbindStreamingPort  		"uSP"

// extended protocol
#define OPDI_getAllPortStates			"gAPS"

#define OPDI_getExtendedPortInfo		"gEPI"
#define OPDI_extendedPortInfo			"EPI"

#define OPDI_getGroupInfo				"gGI"
#define OPDI_groupInfo					"GI"
#define OPDI_getExtendedGroupInfo		"gEGI"
#define OPDI_extendedGroupInfo			"EGI"
