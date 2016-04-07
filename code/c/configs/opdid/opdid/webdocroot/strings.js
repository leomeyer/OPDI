
// split the given property string (prop1=val1;prop2=val2;...)
function getProperties(sConfigP) {
	// loop over the string
	var pos = 0;		// position
	var ch = '\0';	// current character
	var ch1 = '\0';	// lookahead character
	var part = null;
	var key = null;
	var result = {};
		
	while (pos < sConfigP.length)
	{
		ch = sConfigP.charAt(pos);
		if (pos < sConfigP.length - 1)
		{
			ch1 = sConfigP.charAt(pos + 1);
		}
		else
		{
			ch1 = '\0';
		}
		// in part?
		if (part != null)
		{
			// end of key?
			if (ch == '=')
			{
				key = part.trim();
				part = null;
			} 
			// end of value?
			else if (ch == ';')
			{
				if (key != null)
				{
					// store key => value pair
					result[key] = part;
				}
				else
				{
					// store key => empty string
					result[key] = "";
				}
				key = null;
				part = null;
			}
			// escape char?
			else if (ch == '\\')
			{
				// evaluate next character
				if (ch1 == '\\')
				{
					part += '\\';
				} 
				else
				if (ch1 == '=')
				{
					part += '=';
				} 
				else
				if (ch1 == ';')
				{
					part += ';';
				} 
				else
				if (ch1 == 't')
				{
					part += '\t';
				} 
				else
				if (ch1 == 'n')
				{
					part += '\n';
				} 
				else
				if (ch1 == 'r')
				{
					part += '\r';
				} 
				else
				// ! EOS?
				if (ch1 != 0)
				{
					part += ch1;
				}
				// skip next character
				pos++;
			}
			// normal character
			else
				part += ch;
		}
		else
		// part starting
		{
			part = "";
			// end of key?
			if (ch == '=')
			{
				// syntax error, ignore
			} 
			// end of value?
			else if (ch == ';')
			{
				// syntax error, ignore
			}
			// escape char?
			else if (ch == '\\')
			{
				// evaluate next character
				if (ch1 == '\\')
				{
					part += '\\';
				} 
				else
				if (ch1 == '=')
				{
					part += '=';
				} 
				else
				if (ch1 == ';')
				{
					part += ';';
				} 
				else
				if (ch1 == 't')
				{
					part += '\t';
				} 
				else
				if (ch1 == 'n')
				{
					part += '\n';
				} 
				else
				if (ch1 == 'r')
				{
					part += '\r';
				} 
				else
				// ! EOS?
				if (ch1 != 0)
				{
					part += ch1;
				}
				// skip next character
				pos++;
			}
			// normal character
			else
				part += ch;			
		}
		pos++;
	}
		
	// last property unterminated?
	if (key != null) 
	{
		result[key] = (part == null ? "" : part);
	}
	// key not followed by =
	else if (part != null)
	{
		result[key] = "";
	}
		
	return result;
}
