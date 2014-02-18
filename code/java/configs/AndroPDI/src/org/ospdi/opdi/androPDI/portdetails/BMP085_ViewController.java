package org.ospdi.opdi.androPDI.portdetails;

import org.ospdi.opdi.androPDI.AndroPDI;
import org.ospdi.opdi.androPDI.gui.Panel;
import org.ospdi.opdi.drivers.BMP085_Driver;
import org.ospdi.opdi.interfaces.IDriver;
import org.ospdi.opdi.ports.StreamingPort;
import org.ospdi.opdi.androPDI.R;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Shader.TileMode;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewStub;
import android.widget.TextView;

/** View controller for the BMP085 pressure sensor view
 * 
 * @author Leo
 *
 */
public class BMP085_ViewController implements IViewController {
	
	private static final int VIEW_ID = R.layout.bmp085_portview;
	private static final int SLEEP_DELAY = 500;
	
	StreamingPort port;
	BMP085_Driver driver;
	View view;
	Panel panelTemp;
	Panel panelPres;
	TextView tvTemp;
	TextView tvPres;
	boolean bound;
	UpdateThread uThread;
    private Handler mHandler;

	@Override
	public boolean bind(StreamingPort port) {
		this.port = port;
		// get driver
		IDriver driver = port.getDriver();
		if (driver == null) return false;
		if (!(driver instanceof BMP085_Driver)) return false;
		this.driver = (BMP085_Driver) driver;
		
		// bind the port
		// the driver will now receive data
		try {
			port.bind();
		} catch (Exception e) {
			Log.e(AndroPDI.MASTER_NAME, "Could not bind the port: " + port.getID(), e);
			return false;
		}
		bound = true;
		
		return true;
	}

	@Override
	public View inflateLayout(ViewStub stub, Handler handler) {
		mHandler = handler;
		stub.setLayoutResource(VIEW_ID);
		view = stub.inflate();
		view.setMinimumWidth(200);
		view.setMinimumHeight(200);
		panelTemp = (Panel)view.findViewById(R.id.sv_temp);
		panelPres = (Panel)view.findViewById(R.id.sv_pres);
		tvTemp = (TextView) view.findViewById(R.id.bmp085_temp);
		tvPres = (TextView) view.findViewById(R.id.bmp085_pres);

		// start the update thread
		uThread = new UpdateThread();
		uThread.start();
		
		return view;
	}

	@Override
	public boolean createContextMenu(Menu menu, MenuInflater inflater) {
		// TODO Auto-generated method stub
		return false;
	}
	
	@Override
	public synchronized void unbind() {
		// unbind the port
		try {
			port.unbind();
		} catch (Exception e) {
			Log.e(AndroPDI.MASTER_NAME, "Could not unbind the port: " + port.getID(), e);
		}
		bound = false;
	}

	/** The thread that performs regular view updates.
	 * Warning: This is a kludge and needs to be redone.
	 * Thermometer and Barometer classes should handle the complete draw by themselves.
	 * 
	 * @author Leo
	 *
	 */
	private class UpdateThread extends Thread {

		@Override
		public void run() {

			while (bound) {
				mHandler.post(new Runnable() {
					private Thermometer thermometer;
					private Barometer barometer;

					public void run() {
						int maxAge = 5000;
						long age = driver.getDataAge();
						if (age <= 0) age = 1;
						if (age > maxAge) age = maxAge;
						if (!driver.hasValidData() || age >= maxAge) {
							//int color = Color.DKGRAY;
							panelTemp.setBackgroundColor(Color.LTGRAY);
							panelPres.setBackgroundColor(Color.LTGRAY);
							tvTemp.setText("---");
							tvPres.setText("---");
							
						} else { 
							// data is valid

							panelTemp.setBackgroundColor(Color.LTGRAY);
							panelPres.setBackgroundColor(Color.LTGRAY);
							
							// add values
							tvTemp.setText("" + driver.getTemperature() + " °C");
							tvPres.setText("" + driver.getPressure() + " hPa");
						}
						if (thermometer == null) {
							thermometer = new Thermometer(panelTemp.getWidth(), panelTemp.getHeight());
							panelTemp.setRunOnDraw(new Panel.RunOnDraw() {
								
								@Override
								public void drawOn(Canvas canvas) {
									// draw thermometer inside (color gradient)
									// save clipping region							
									canvas.save(Canvas.CLIP_SAVE_FLAG);
								    canvas.clipPath(thermometer);

								    Paint p = new Paint();
								    RectF bounds = new RectF();
								    thermometer.computeBounds(bounds, true);
								    float hx = (bounds.right - bounds.left) / 2;
								    // draw plain background
								    p.setColor(Color.LTGRAY);
								    canvas.drawPaint(p);		
								    
								    // draw colored background
								    // height scale temperature
								    // top is 100 °C
								    // bottom is -100 °C
								    float temp = driver.getTemperature();
								    if (temp > 100) temp = 100;
								    if (temp < -100) temp = -100;
								    float top = (float) ((bounds.bottom - bounds.top) / 2 - ((bounds.bottom - bounds.top) / 200.0 * temp));
								    // linear gradient from red to blue
								    int topcolor = Color.rgb(128 + (int)(128.0 / 100 * temp), 0, 128 - (int)(128.0 / 100 * temp));
								    p.setShader(new LinearGradient(
								    		hx, bounds.top + top, hx, bounds.bottom, topcolor, Color.BLUE, TileMode.MIRROR));
								    // clip
								    canvas.clipRect(0, bounds.top + top, canvas.getWidth(), bounds.bottom);
								    canvas.drawPaint(p);		

									// restore clipping region							
								    canvas.restore();
								    
								    p = new Paint();
								    p.setStrokeWidth(2.0f);
								    p.setColor(Color.WHITE);
								    p.setStyle(Paint.Style.STROKE);
									canvas.drawPath(thermometer, p);
								}
							});
							if (barometer == null) {
								barometer = new Barometer(panelPres.getWidth(), panelPres.getHeight());
								panelPres.setRunOnDraw(new Panel.RunOnDraw() {
									
									@Override
									public void drawOn(Canvas canvas) {
									    Paint p = new Paint();

										// save clipping region							
										canvas.save(Canvas.CLIP_SAVE_FLAG);
									    canvas.clipPath(barometer);

									    RectF bounds = new RectF();
									    thermometer.computeBounds(bounds, true);
									    // draw plain background
									    p.setColor(Color.LTGRAY);
									    canvas.drawPaint(p);		
									    
									    // draw colored background
									    // height scale pressure
									    // top is 1100 hPa
									    // bottom is 300 hPa
									    float pres = driver.getPressure();
									    if (pres > 1100) pres = 1100;
									    if (pres < 300) pres = 300;
									    float top = (float) (bounds.bottom - (bounds.height() / 800.0 * (pres - 300)));
									    // silver color
									    p.setColor(Color.DKGRAY);
									    // clip
									    canvas.clipRect(0, bounds.top + top, canvas.getWidth(), bounds.bottom);
									    canvas.drawPaint(p);		

										// restore clipping region							
									    canvas.restore();

									    p = new Paint();
									    p.setStrokeWidth(2.0f);
									    p.setColor(Color.WHITE);
									    p.setStyle(Paint.Style.STROKE);
										canvas.drawPath(barometer, p);
									}
								});
							}
						}
					}
                });
				
				try {
					Thread.sleep(SLEEP_DELAY);
				} catch (InterruptedException e) {
					interrupt();
					return;
				}
			}
		}
	}
	
	private class Thermometer extends Path {
		
		private float scale = 1.1f;
		float toppos = 20;
		float basewidth;
		float baseheight; 
		
		public Thermometer(int width, int height) {
			super();
			int hx = width / 2;
			
			basewidth = width / 6.0f * scale;
			baseheight = height * 0.6f * scale; 
			
			// top arc
			float arcwidth = basewidth;
			float archeight = baseheight / 7;
			RectF rect = new RectF(hx - basewidth / 2, toppos, 
					hx - basewidth / 2 + arcwidth,
					toppos + archeight);
			this.moveTo(rect.right, toppos + archeight / 2);
			this.arcTo(rect, 0, -180);
			
			float px = hx - basewidth / 2;
			float py = baseheight;
			// left line down
			this.lineTo(px, py);
			
			float r = basewidth / (float)(2 * Math.sin(Math.PI / 4));
			float h = r * (float)Math.cos(Math.PI / 4);
			
			rect = new RectF(hx - r, py - r + h, hx + r, py + r + h);
			// bottom arc
			this.arcTo(rect, 225, -270);
			
			this.moveTo(hx + basewidth / 2, py);

			px = hx + basewidth / 2;
			py = toppos + archeight / 2;
			// right line up
			this.lineTo(px, py);

			this.setLastPoint(px, py);
		}
		
	}

	private class Barometer extends Path {
		
		private float scale = 1.1f;
		float toppos = 20;
		float basewidth;
		float baseheight; 
		
		public Barometer(int width, int height) {
			super();
			int hx = width / 3;
			
			basewidth = width / 6.0f * scale;
			baseheight = height * 0.6f * scale; 
			
			// top arc
			float arcwidth = basewidth;
			float archeight = baseheight / 7;
			RectF rect = new RectF(hx - basewidth / 2, toppos, 
					hx - basewidth / 2 + arcwidth,
					toppos + archeight);
			this.moveTo(rect.right, toppos + archeight / 2);
			this.arcTo(rect, 0, -180);
			
			float px = hx - basewidth / 2;
			float py = baseheight * 0.8f;
			// left line down
			this.lineTo(px, py);
			
			float bpx = px;
			float bpy = py;
			
			rect = new RectF(px, py, px + basewidth * 3, py + basewidth * 2);
			// bottom arc
			this.arcTo(rect, 180, -180);
			
			px = rect.right;
			py = rect.top + rect.height() / 2 - baseheight * 0.1f;
			// right bow
			rect = new RectF(px - basewidth * 0.1f, py, px + basewidth * 0.1f, py + baseheight * 0.1f);
			this.arcTo(rect, 90, -180);
			
			// small right line
			this.lineTo(px, rect.top - baseheight * 0.1f);
			
			px -= basewidth * 0.5f;
			// top line (path must be closed :-)
			this.lineTo(px, rect.top - baseheight * 0.1f);
			// small left line
			this.lineTo(px, py);
			
			// left bow
			rect = new RectF(px - basewidth * 0.1f, py, px + basewidth * 0.1f, py + baseheight * 0.1f);
			this.arcTo(rect, 270, -180);
			
			// upper bottom arc
			rect = new RectF(bpx + basewidth, bpy + basewidth, px + basewidth * 0.05f, bpy + basewidth + baseheight * 0.1f);
			this.arcTo(rect, 0, 180);
			
			px = hx + basewidth / 2;
			py = toppos + archeight / 2;
			// right line up
			this.lineTo(px, py);
			this.setLastPoint(px, py);
		}
		
	}
}
