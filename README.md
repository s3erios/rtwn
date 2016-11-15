# rtwn
Merged rtwn(4), urtwn(4) and urtwm FreeBSD drivers   
(in HEAD since https://svnweb.freebsd.org/base?view=revision&revision=307529)   
Should be compatible with 11.0-RELEASE   

### **How-to-build:**

1) Clone / download this repository.  
2) Apply 'patch-usbdevs.diff' to your source tree checkout; e.g.  
  *cd /usr/src/ && svn patch \<patch-usbdevs.diff location\>*  
3) Build and install firmware:  
   *cd \<repository location\>/sys/modules/rtwnfw && make depend && make && make install*  
4) Build and install main module:  
   *cd ../rtwn && make depend && make && make install*  
5) Bus-specific modules:   
    - USB (RTL8188CUS/RTL8192CU/RTL8188EU/RTL8812AU/RTL8821AU):   
   *cd ../rtwn_usb && make depend && make && make install*   
    - PCI (RTL8188CE):    
    *cd ../rtwn_pci && make depend && make && make install*   
     
     
   
### **How-to-test:**  
1) Load the driver:  
   *kldload if_rtwn*  
   *kldload if_rtwn_usb* (or *kldload if_rtwn_pci*)   
   (You may need to unload/move old urtwn(4)/rtwn(4) drivers before this step)   
   
In case if device was recognized successfully driver will report about that:  
> rtwn0: <802.11n WLAN Adapter> on usbus4  
> rtwn0: MAC/BB RTL8821AU, RF 6052 1T1R

or  
> rtwn0: MAC/BB RTL8812AU, RF 6052 2T2R  

2) Load necessary modules (if not already loaded):  
   *kldload wlan_amrr wlan_ccmp wlan_tkip wlan_wep*  
3) Create wlan(4) interface:  
   *ifconfig wlan1 create wlandev rtwn0*  
3) Start wpa_supplicant(8):  
   wpa_supplicant -i wlan1 -c /etc/wpa_supplicant.conf  
4) Start dhclient(8) after association / 4-Way handshake:  
   *dhclient wlan1*  
