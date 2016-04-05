SET COPYCMD=copy
SET BASEFOLDER=..\..\..\..\

%COPYCMD% %BASEFOLDER%common\opdi_message.c .
%COPYCMD% %BASEFOLDER%common\opdi_port.c .
%COPYCMD% %BASEFOLDER%common\opdi_protocol.c .
%COPYCMD% %BASEFOLDER%common\opdi_slave_protocol.c .
%COPYCMD% %BASEFOLDER%common\opdi_strings.c .
%COPYCMD% %BASEFOLDER%common\opdi_config.h .
%COPYCMD% %BASEFOLDER%common\opdi_constants.h .
%COPYCMD% %BASEFOLDER%common\opdi_message.h .
%COPYCMD% %BASEFOLDER%common\opdi_port.h .
%COPYCMD% %BASEFOLDER%common\opdi_protocol.h .
%COPYCMD% %BASEFOLDER%common\opdi_protocol_constants.h .
%COPYCMD% %BASEFOLDER%common\opdi_slave_protocol.h .
%COPYCMD% %BASEFOLDER%common\opdi_strings.h

%COPYCMD% %BASEFOLDER%platforms\opdi_platformfuncs.h
%COPYCMD% %BASEFOLDER%platforms\avr\opdi_platformfuncs.c
%COPYCMD% %BASEFOLDER%platforms\avr\opdi_platformtypes.h

%COPYCMD% %BASEFOLDER%configs\ArduinoPDI\OPDI.h
%COPYCMD% %BASEFOLDER%configs\ArduinoPDI\OPDI.cpp
%COPYCMD% %BASEFOLDER%configs\ArduinoPDI\ArduinoPDI.h
%COPYCMD% %BASEFOLDER%configs\ArduinoPDI\ArduinoPDI.cpp
