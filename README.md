# inclinometer-display
Device to display output from measurement specialties inclinometer

This device provides a graphical display of inclinometer output for leveling truck mounted RADAR systems (and anything else
one might desire)

Hardware requirements:
     Arduino DUE or similar.  I used a sainsmart clone
     
     MAX3232 on tx1/rx1 and tx2/rx2 for RS-232 level shifting. 
     one option: sparkfun transceiver breakout - MAX3232.  P/N BOB-11189
     https://www.sparkfun.com/products/11189
     
     DB9 connecters (male on tx2/rx2, female for tx1/rx1 passthru)
     
     measurement specialties NS-5/DMG2-S inclinometer
     
     sainsmart LCD driver and LCD panel SKU: 20-011-D20  
     https://www.sainsmart.com/products/sainsmart-due-5-lcd-touch-panel-sd-card-slot-tft-lcd-shield-kit-for-arduino
     
     12VDC, 500mA power supply with barrel connector -or- 5VDC, 1A with micro USB connector
     
Software requirements:
     GroteskBold32x64.c was downloaded from http://www.rinkydinkelectronics.com/r_fonts.php
     The excellent UTFT library from http://www.rinkydinkelectronics.com/library.php?id=51
     
     Note for UTFT installation/configuration, usage with CTE DUE shield:
         1.uncomment "#define CTE_DUE_SHIELD 1" in the HW_ARM_defines.h in the \hardware\arm folder of the UTFT library
         2.Change the pinout to : UTFT myGLCD(CTE50,25,26,27,28);
  
System description:
  
  The Arudino DUE is connected to the MAX3232 to provide two RS-232 serial ports.  One port is connected to the inclinometer device,
  the other port provides a transparent passthru for an attached PC (for configuration of the inclinometer and/or monitoring with
  the PC software)
  
  On boot, the Arudino code attempts to order the inclinometer to begin sending measurements continuously.  It is assumed that the
  inclinometer has been previously set to provide 2 Hz updates, appropriate filtering settings, and a 9600 baud rate.  Without the
  correct baud rate, the PC passthru port will not work.  The PC  must also be set to 9600 baud.  The inclinometer also has 
  programmable level offsets, so it will report X=0 and Y=0 when the truck is in the desired orientation (i.e. level)
  
  The software then eavesdrops on the incoming "X=+8.888" and "Y=+8.888" format messages and displays them in a digital display, as
  well as a graphical dot on a ring/crosshair display.  The dot position is not linear, the graph is scaled with a sigmoid function so
  the center of the graph is more sensitive than the outside edges.  After I install in the truck, i need to tweak LEVEL_SIGMOID
  so the system is easy to use, and maybe plot a degree scale on the concentric rings.
  
  If the software doesn't hear a new message within so many milliseconds (INCLINOMETER_TIMEOUT), the possibly misleading dot is
  removed from the display, the X/Y angles are set to a MISSING_VALUE (9.999) and a flashing message is presented (NOT UPDATING). 
  This would usually be caused by the inclinometer data cable being unplugged, or the power is not on to the inclinometer.
  
  Just FYI version 1.0 had a 2x20 dot matrix LCD display, and vastly inferior in many other ways :)
  
CHANGELOG

3/9/2018:  inclinometer3.ino has a landscape display instead of portrait

