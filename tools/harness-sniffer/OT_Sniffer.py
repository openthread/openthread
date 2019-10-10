'''OpenThread Sniffer API implementation'''

import subprocess
from GRLLibs.UtilityModules.ModuleHelper import ModuleHelper
from ISniffer import ISniffer


class OT_Sniffer(ISniffer):
    common_speed = [460800, 115200, 9600]

    def __init__(self, **kwargs):
        try:
            self.channel = kwargs.get('channel')
            self.port = kwargs.get('addressofDevice')
            self.baudrate = None
            self.subprocess = None
            self.is_active = False

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('OT_Sniffer: [intialize] --> ' + str(e))

    def discoverSniffer(self):
        sniffers = []

        p_discover = subprocess.Popen('extcap_ot.bat --extcap-interfaces', stdout=subprocess.PIPE, shell=True)
        for line in p_discover.stdout.readlines():
           if line.startswith('interface'):
               try:
                   interface_port = line[line.index('value=')+6: line.index('}{display')]
                   sniffers.append(OT_Sniffer(addressofDevice=interface_port, channel=ModuleHelper.Default_Channel))
               except Exception, e:
                   ModuleHelper.WriteIntoDebugLogger('OT_Sniffer: [discoverSniffer] --> Error: ' + str(e))

        p_discover.wait()
        return sniffers

    def startSniffer(self, channelToCapture, captureFileLocation):
        """
        Method for starting the sniffer capture on given channel and this should create wireshark 'pcapng' file at the given location. Capture should happen in background so that method call will be non-blocking and asynchronous.
        @param channelToCapture : int : channel number to start the capture
        @param captureFileLocation : string : Full path with the filename with extension is passed.
        """
        try:
            # baudrate auto detection
            cmd_baudrate = ['extcap_ot.bat', '--extcap-interface', self.port,'--extcap-baudrate']
            p_baudrate = subprocess.Popen(cmd_baudrate, stdout=subprocess.PIPE, shell=True)

            for line in p_baudrate.stdout.readlines():
                if (line.startswith('baudrate')):
                    self.baudrate = line.split(':')[1][:6]

            p_baudrate.wait()

            # start sniffer
            if self.baudrate is not None:
                self.setChannel(channelToCapture)
                p_where = subprocess.Popen('where python', stdout=subprocess.PIPE, shell=True)
                pythonExe = p_where.stdout.readline().replace("\r\n", "")

                if pythonExe.endswith(".exe"):
                    snifferPy = pythonExe[:-10]+'Scripts\sniffer.py'

                    cmd = [pythonExe, snifferPy,
                           '-c', str(self.channel),
                           '-u', str(self.port),
                           '-b', str(self.baudrate),
                           '--crc',
                           '-o', captureFileLocation]
                    self.is_active = True
                    ModuleHelper.WriteIntoDebugLogger('OT_Sniffer: [cmd] --> %s' % str(cmd))
                    self.subprocess = subprocess.Popen(cmd)

            else:
                ModuleHelper.WriteIntoDebugLogger('OT_Sniffer: [startSniffer] --> Wrong baudrate!')

        except Exception, e:
            ModuleHelper.WriteIntoDebugLogger('OT_Sniffer: [startSniffer] --> Error: ' + str(e))


    def stopSniffer(self):
        """
        Method for ending the sniffer capture.
        Should stop background capturing, No furthur file I/O should happen in capture file.
        """
        if self.is_active:
            self.is_active = False
            if self.subprocess:
                self.subprocess.terminate()
                self.subprocess.wait()
     
    def setChannel(self, channelToCapture):
        """
        Method for changing sniffer capture
        @param channelToCapture : int : channel number is passed to change the channel which is set during the constructor call. 
        """
        self.channel = channelToCapture


    def getChannel(self):
        """
        Method to query sniffer for the channel it is sniffing on
        @return : int : current capture channel of this sniffer instance.
        """
        return self.channel


    def validateFirmwareVersion(self, addressofDevice):
        """
        Method to query sniffer firmware version details.
        Shall be used while discoverSniffer() to validate the sniffer firmware.
        @param addressofDevice : string : serial com port or IP address, shall be None if need to verify own fireware version.
        @return : bool : True if expected firmware is found , False if not
        """
        return True


    def isSnifferCapturing(self):
        """
        method that will return true when sniffer device is capturing
        @return : bool 
        """
        return self.is_active

     
    def getSnifferAddress(self):
        """
        Method to retrun the current sniffer's COM/IP address
        @return : string
        """

        return self.port

    def globalReset(self):
        """Method to reset all the global and class varibaled"""
        pass
