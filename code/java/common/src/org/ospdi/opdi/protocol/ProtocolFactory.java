package org.ospdi.opdi.protocol;

import java.lang.reflect.Constructor;
import java.util.HashMap;

import org.ospdi.opdi.interfaces.IBasicProtocol;
import org.ospdi.opdi.interfaces.IDevice;

/** This class is a protocol factory that selects the proper protocol for a device.
 * It supports registering protocols that are not implemented in the library itself.
 * A protocol class must provide a constructor with parameters (IDevice device, IConnectionListener listener).
 * 
 * @author Leo
 *
 */
public final class ProtocolFactory {
	
	private static HashMap<String, Class<? extends IBasicProtocol>> registeredProtocols = new HashMap<String, Class<? extends IBasicProtocol>>();
	
	static {
		// register basic protocol
		registerProtocol(BasicProtocol.MAGIC, BasicProtocol.class);
		// register extended protocol
		registerProtocol(ExtendedProtocol.MAGIC, ExtendedProtocol.class);
	}
	
	public static void registerProtocol(String magic, Class<? extends IBasicProtocol> clazz) {
		// check that class is valid
		try {
			Constructor<? extends IBasicProtocol> cons = clazz.getConstructor(IDevice.class);
			if (cons == null)
				throw new IllegalArgumentException("Constructor with parameter IDevice not found");
		} catch (Exception e) {
			throw new IllegalArgumentException("Class " + clazz.getName() + " can't be used as protocol implementation", e);
		}		
		// the class is ok
		synchronized(registeredProtocols) {
			registeredProtocols.put(magic, clazz);
		}
	}
	
	public static IBasicProtocol getProtocol(IDevice device, String magic) {
		
		Class<? extends IBasicProtocol> clazz = null;
		synchronized(registeredProtocols) {
			clazz = registeredProtocols.get(magic);
		}
		if (clazz != null) {
			// matching class found; get constructor
			try {
				Constructor<? extends IBasicProtocol> cons = clazz.getConstructor(IDevice.class);
				if (cons == null)
					throw new IllegalArgumentException("Constructor with parameter IDevice not found");
				
				// instantiate it
				return cons.newInstance(device);
			} catch (Exception e) {
				throw new IllegalArgumentException("Class " + clazz.getName() + " can't be used as protocol implementation", e);
			}
		}
		return null;
	}
	
	// This class can't be instantiated
	private ProtocolFactory() {}
}
