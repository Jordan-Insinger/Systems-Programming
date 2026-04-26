# Assignment6b
Jordan Insinger \
Advanced Systems Programming \
26 April 2026


This usbkbd driver added a backdoor which can be activated by pressing the numlock key three times. I found this to be 0x53, I assume that's the same for all keyboards. Once the backdoor is enabled every new odd numbered led urb will be dropped. I interpreted this to mean that the numlock press that enables the backdoor doesn't count as a "new led press". Therefore the next led urb after backdoor is enabled will be dropped. 

As showcased in the demo even if the led urb is dropped the bit signifying that it had been pressed is still flipped, thus if you press a different led key after you drop the previous, you can see the led light toggle as the new led urb goes through.

The driver can be compiled using my makefile, running "make all". I included the script to unbind the hid driver and bind the kbd driver, though I think on different machines the keyboard tag could be different. Mine is 1-2:1.0, I believe I've seen 2-1:1.0 as well. 