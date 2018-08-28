#!/usr/bin/env python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from PyQt4 import uic
import types

import immp

from frame import Frame
from plot import Plot

def lineKeyPressEvent(self, event):
    if event.key() == Qt.Key_Up:
        if self.historyPos < len(self.history):
            self.historyPos = self.historyPos + 1
            self.setText(self.history[-self.historyPos])
    elif event.key() == Qt.Key_Down:
        if self.historyPos > 0:
            self.historyPos = self.historyPos - 1
        if self.historyPos == 0:
            self.setText("")
        else:
            self.setText(self.history[-self.historyPos])
    else:
        if event.key() == Qt.Key_Return:
            if not len(self.history) or (self.text() and self.text() != self.history[-1]):
                self.history.append(self.text())
            self.historyPos = 0
        QLineEdit.keyPressEvent(self, event)


class FastGUI(QMainWindow):
    def __init__(self):
        super(FastGUI, self).__init__()

        ## UI
        self.ui = uic.loadUi("gui/mainwindow.ui", self)

        ## Timer for periodic request of daemon state
        self.timer = QTimer()
        self.timer.setSingleShot(True)
        self.connect(self.timer, SIGNAL("timeout()"), self.slotTimerEvent)

        ## Timer for periodic request of current image
        self.timerImage = QTimer()
        self.timerImage.setSingleShot(True);
        self.connect(self.timerImage, SIGNAL("timeout()"), self.slotTimerImageEvent)

        ## Socket connection to daemon
        self.socket = QTcpSocket()
        self.hw_host = "localhost"
        self.hw_port = 5555
        self.connect(self.socket, SIGNAL("connected()"), self.slotConnected)
        self.connect(self.socket, SIGNAL("disconnected()"), self.slotDisconnected)
        self.connect(self.socket, SIGNAL("readyRead()"), self.slotReadyRead)
        self.connect(self.socket, SIGNAL("error(QAbstractSocket::SocketError)"), self.slotSocketError)
        self.binaryMode = False
        self.binaryLength = 0
        self.binaryType = ""
        self.socketBuffer = ""

        ## Current image view
        self.currentImage = Frame()
        self.currentImage.setWindowTitle("Current Frame")
        self.connect(self.currentImage, SIGNAL("roiChanged"), self.slotRoiChanged)
        self.connect(self.currentImage, SIGNAL("roiReset"), self.slotRoiReset)

        ## Running image view
        self.runningImage = Frame()
        self.runningImage.setWindowTitle("Running Average Frame")
        self.connect(self.runningImage, SIGNAL("roiChanged"), self.slotRoiChanged)
        self.connect(self.runningImage, SIGNAL("roiReset"), self.slotRoiReset)

        ## Total image view
        self.totalImage = Frame()
        self.totalImage.setWindowTitle("Cumulative Frame")
        self.connect(self.totalImage, SIGNAL("roiChanged"), self.slotRoiChanged)
        self.connect(self.totalImage, SIGNAL("roiReset"), self.slotRoiReset)

        ## Flux plot
        self.fluxPlot = Plot()
        self.fluxPlot.setWindowTitle("Full Frame Flux / 1000 frames")
        self.fluxPlot.maxLength = 1000

        self.totalPlot = Plot()
        self.totalPlot.setWindowTitle("Full Frame Flux / complete")

        ## Buttons
        self.connect(self.ui.pushButtonAcquisitionStart, SIGNAL("clicked()"),
                     lambda : self.sendCommand("start"))
        self.connect(self.ui.pushButtonAcquisitionStop, SIGNAL("clicked()"),
                     lambda : self.sendCommand("stop"))
        self.connect(self.ui.pushButtonStorageStart, SIGNAL("clicked()"),
                     lambda: self.sendCommand("storage_start object=" + self.ui.lineEditObject.text()))
        self.connect(self.ui.pushButtonStorageStop, SIGNAL("clicked()"),
                     lambda: self.sendCommand("storage_stop"))

        ## Countdown
        self.ui.lineEditCountdown.setValidator(QIntValidator(0, 1000000))
        self.connect(self.ui.lineEditCountdown, SIGNAL("returnPressed()"),
                     lambda: self.sendCommand("set_countdown " + str(self.ui.lineEditCountdown.text())))

        ## Command line
        self.connect(self.ui.lineEditCommand, SIGNAL("returnPressed()"),
                     lambda: self.sendCommand(self.ui.lineEditCommand.text()))

        ## Check boxes
        self.connect(self.ui.checkBoxCurrentImage, SIGNAL("stateChanged(int)"),
                     lambda i: self.currentImage.show() if i else self.currentImage.hide())
        self.connect(self.currentImage, SIGNAL("closed()"),
                     lambda : self.ui.checkBoxCurrentImage.setCheckState(False))

        self.connect(self.ui.checkBoxRunningImage, SIGNAL("stateChanged(int)"),
                     lambda i: self.runningImage.show() if i else self.runningImage.hide())
        self.connect(self.runningImage, SIGNAL("closed()"),
                     lambda : self.ui.checkBoxRunningImage.setCheckState(False))

        self.connect(self.ui.checkBoxTotalImage, SIGNAL("stateChanged(int)"),
                     lambda i: self.totalImage.show() if i else self.totalImage.hide())
        self.connect(self.totalImage, SIGNAL("closed()"),
                     lambda : self.ui.checkBoxTotalImage.setCheckState(False))

        self.connect(self.ui.checkBoxFlux, SIGNAL("stateChanged(int)"),
                     lambda i: self.fluxPlot.show() if i else self.fluxPlot.hide())
        self.connect(self.fluxPlot, SIGNAL("closed()"),
                     lambda : self.ui.checkBoxFlux.setCheckState(False))

        self.connect(self.ui.checkBoxTotalFlux, SIGNAL("stateChanged(int)"),
                     lambda i: self.totalPlot.show() if i else self.totalPlot.hide())
        self.connect(self.totalPlot, SIGNAL("closed()"),
                     lambda : self.ui.checkBoxTotalFlux.setCheckState(False))

        self.connect(self.ui.actionReconnectToDaemon, SIGNAL("activated()"), self.slotReconnect)
        self.connect(self.ui.actionRestartImageTimer, SIGNAL("activated()"), self.slotTimerImageEvent)

        self.connect(self.ui.actionCommandsExit, SIGNAL("activated()"), lambda: self.sendCommand("exit"))

        self.connect(self.ui.spinBoxPostprocess, SIGNAL("valueChanged(int)"),
                     lambda i: self.sendCommand("set_postprocess %d" % (i)))
        #self.connect(self.ui.doubleSpinBoxExposure, SIGNAL("valueChanged(double)"),
        #             lambda d: self.sendCommand("set_grabber exposure=%g" % (d)))
        self.connect(self.ui.doubleSpinBoxExposure, SIGNAL("editingFinished()"),
                     lambda : self.sendCommand("set_grabber exposure=%g" % (self.ui.doubleSpinBoxExposure.value())))

        ## Initial focus to command line
        self.ui.lineEditCommand.setFocus()
        self.ui.lineEditCommand.history = []
        self.ui.lineEditCommand.historyPos = 0
        self.ui.lineEditCommand.keyPressEvent = types.MethodType(lineKeyPressEvent, self.ui.lineEditCommand)

    def closeEvent(self, event):
        self.emit(SIGNAL("closed()"))
        QMainWindow.closeEvent(self, event)

    def isConnected(self):
        return self.socket.state() == QAbstractSocket.ConnectedState

    def sendCommand(self, command):
        if not command:
            return

        if self.isConnected():
            string = command.__str__()
            self.socket.write(string)
            self.socket.write('\0')

    def setHost(self, host = 'localhost', port = 5555):
        self.hw_host = host
        self.hw_port = port
        self.ui.setWindowTitle("FAST Controller @ " + host + ":" + str(port))
        self.slotDisconnected()

    def slotConnect(self):
        self.socket.connectToHost(self.hw_host, self.hw_port)

    def slotConnected(self):
        self.ui.labelConnected.setText("<font color='green'>Connected</font>")
        self.timer.start(100)
        self.timerImage.start(1000)

    def slotReconnect(self):
        if self.isConnected():
            self.socket.disconnectFromHost()
        else:
            self.slotDisconnected()

    def slotDisconnected(self):
        self.ui.labelConnected.setText("<font color='red'>Disconnected</font>")
        self.ui.labelAcquisition.setText("<font color='red'>unknown</font>")
        self.ui.labelObject.setText("unknown")
        self.ui.labelStorage.setText("<font color='red'>unknown</font>")
        self.timer.stop()
        self.timerImage.stop()
        self.socketBuffer = ""
        self.slotConnect()

    def slotSocketError(self, error):
        if error == QTcpSocket.RemoteHostClosedError or error == QTcpSocket.ConnectionRefusedError:
            ## FIXME: Potentially we may start several such timers concurrently
            QTimer.singleShot(1000, self.slotConnect)
        self.timer.stop()

    def slotTimerEvent(self):
        self.sendCommand("get_status")
        self.timer.start(100)

    def slotTimerImageEvent(self):
        if self.isConnected():
            self.sendCommand("get_current_frame")
            self.sendCommand("get_total_frame")
            self.sendCommand("get_running_frame")
        else:
            self.timerImage.start(1000)

    def slotReadyRead(self):
        while self.socket.size():
            if self.binaryMode:
                if self.socket.size() >= self.binaryLength:
                    buffer = self.socket.read(self.binaryLength)
                    self.binaryMode = False
                    self.binaryLength = 0

                    i = QImage()
                    i.loadFromData(buffer)
                    if not i.isNull():
                        if self.binaryType == 'current_frame':
                            self.currentImage.setImage(i)
                        elif self.binaryType == 'total_frame':
                            self.totalImage.setImage(i)
                        elif self.binaryType == 'running_frame':
                            self.runningImage.setImage(i)

                    ## FIXME: just in case
                    self.timerImage.start(1000)
                else:
                    break
            else:
                string = self.socketBuffer
                stringCompleted = False

                while self.socket.size():
                    (result, ch) = self.socket.getChar()

                    if result and ch and ch != '\0' and ch != '\n':
                        string += ch
                    elif result:
                        stringCompleted = True
                        break
                    else:
                        break

                ## TODO: store incomplete lines for surther processing
                if string and stringCompleted:
                    self.processCommand(string)
                    self.socketBuffer = ""
                else:
                    self.socketBuffer = string

    def slotRoiChanged(self, x1, y1, x2, y2):
        self.sendCommand("set_region x1=" + str(x1) + "y1= " + str(y1) + " x2=" + str(x2) + " y2=" + str(y2))

        for image in [self.currentImage, self.runningImage, self.totalImage]:
            image.setRoi(x1, y1, x2, y2)

    def slotRoiReset(self):
        self.sendCommand("set_region 0 0 0 0")

        for image in [self.currentImage, self.runningImage, self.totalImage]:
            image.resetRoi()

    def updateStatus(self, params):
        # Acquiring
        if int(params.get('acquisition', 0)):
            self.ui.labelAcquisition.setText("<font color='blue'>Acquiring</font>")
            self.ui.pushButtonAcquisitionStart.setEnabled(False)
            self.ui.pushButtonAcquisitionStop.setEnabled(True)
            age = float(params.get('age_acquired', '0'))
            color = "green" if age < 1 else "red"
            self.ui.labelLastAcquired.setText("<font color='%s'>%0.2f</font> s" % (color, age))
        else:
            self.ui.labelAcquisition.setText("<font color='gray'>Idle</font>")
            self.ui.pushButtonAcquisitionStart.setEnabled(True)
            self.ui.pushButtonAcquisitionStop.setEnabled(False)
            self.ui.labelLastAcquired.setText('')

        # Storage
        if int(params.get('storage', 0)):
            self.ui.labelStorage.setText("<font color='blue'>Storage On</font>")
            self.ui.pushButtonStorageStart.setEnabled(False)
            self.ui.pushButtonStorageStop.setEnabled(True)
            age = float(params.get('age_stored', '0'))
            color = "green" if age < 1 else "red"
            self.ui.labelLastStored.setText("<font color='%s'>%0.2f</font> s" % (color, age))
        else:
            self.ui.labelStorage.setText("<font color='gray'>Storage Off</font>")
            self.ui.pushButtonStorageStart.setEnabled(True)
            self.ui.pushButtonStorageStop.setEnabled(False)
            self.ui.labelLastStored.setText('')

        # Object
        self.ui.labelObject.setText(params.get('object', 'unknown'))

        # Countdown
        countdown = int(params.get('countdown', -1))
        self.ui.labelCountdown.setText("<font color='red'>" + str(countdown) + "</font>"
                                       if countdown > 0
                                       else "")

        # Temperature
        temperature = float(params.get('temperature', 0))
        self.ui.labelTemperature.setText(str(temperature) if temperature != 0 else "unknown")
        self.ui.labelTemperatureStatus.setText({"0":"Cooler Off",
                                                "1":"<font color='green'>Stabilized</font>",
                                                "2":"<font color='red'>Cooling</font>",
                                                "3":"Drift",
                                                "4":"<font color='blue'>Not Stabilized</font>",
                                                "5":"Fault"}.get(params.get("temperaturestatus", -1), "unknown"))
        self.ui.labelCooling.setText("<font color='green'>Cooling On</font>"
                                     if params.get('cooling') == '1'
                                     else "<font color='red'>Cooling Off</font>")

        # FPS/Exposure
        self.ui.labelFPS.setText("%.3g" % float(params.get('fps', '0')))
        if params.get('pvcam', '1') == '1':
            self.ui.labelExposure.setText(params.get('exposure', 'unknown'))
        else:
            self.ui.labelExposure.setText(params.get('exposure', 'unknown') + " / " + params.get('actualexposure', 'unknown'))

        if not self.ui.doubleSpinBoxExposure.hasFocus():
            self.ui.doubleSpinBoxExposure.setValue(float(params.get('exposure', 0)))

        # Postprocess
        self.ui.labelPostprocess.setText({"0":"<font color='gray'>Disabled</font",
                                          "1":"<font color='green'>Dark</font>",
                                          "2":"<font color='green'>Dark + Horiz</font>",
                                          "3":"<font color='green'>Dark + Horiz + Vert</font>",
                                      }.get(params.get("postprocess"), "unknown"))
        if not self.ui.spinBoxPostprocess.hasFocus():
            self.ui.spinBoxPostprocess.setValue(int(params.get("postprocess", 0)))
        self.ui.labelDark.setText("<font color='green'>Dark Loaded</font>" if params.get('dark') == '1'
                                  else "<font color='gray'>No dark frame</font>")

        # ROI
        self.labelRoi.setText(params.get("region_x1", "?") + ", " +
                              params.get("region_y1", "?") + " - " +
                              params.get("region_x2", "?") + ", " +
                              params.get("region_y2", "?"))

        # Accumulation
        self.ui.labelAccumulation.setText(params.get('accumulation', ''))


        # Andor
        # self.ui.labelShutter.setText("Rolling" if params.get("shutter", "?") == '0' else "Global")
        # self.ui.labelBinning.setText(params.get('binning', 'unknown'))

        # rate = int(params.get('rate', -1))
        # self.ui.labelRate.setText({"1":"100 MHz", "2":"200 MHz", "3":"280 MHz"}.get(params.get('rate', -1), "unknown"))

        # self.ui.labelReadout.setText(params.get('readout', 'unknown'))
        # self.ui.labelFilter.setText("<font color='red'>Noise Filter</font>"
        #                             if params.get('filter') == '1'
        #                             else "<font color='gray'>No Filter</font>")
        # self.ui.labelOverlap.setText("<font color='green'>Overlap On</font>"
        #                              if params.get('overlap') == '1'
        #                              else "<font color='gray'>Overlap Off</font>")

        # PVCAM
        self.ui.labelPMode.setText({"0":"Normal", "1":"Frame Transfer"}.get(params.get("pvcam_pmode"), "unknown"))
        self.ui.labelClear.setText({"0":"Never", "1":"Pre-Exposure", "2":"Pre-Sequence"}.get(params.get("pvcam_clear"), "unknown"))
        self.ui.labelReadout.setText({"0":"Multiplication", "1":"Normal"}.get(params.get("pvcam_port"), "unknown"))
        self.ui.labelSpeed.setText("%d / %d MHz" % (int(params.get("pvcam_spdtab", 0)), 1e3/float(params.get("pvcam_pixtime", 1))))
        self.ui.labelGain.setText("%d / %d" % (int(params.get("pvcam_gain", 0)), int(params.get("pvcam_mgain", 0))))

    def processCommand(self, string):
        #print "Command:", string

        command = immp.parse(string.__str__())

        if command.name() == 'fast_status':
            self.updateStatus(command.kwargs)

        elif command.name() in ['current_frame', 'total_frame', 'running_frame']:
            self.binaryType = command.name()
            self.binaryMode = True
            self.binaryLength = int(command.kwargs.get('length', 0))

            if command.name() == 'current_frame':
                self.currentImage.setStatus('mean = %g' % float(command.kwargs.get('mean', 0)))
        elif command.name() in ['current_frame_timeout', 'total_frame_timeout', 'running_frame_timeout']:
            self.timerImage.start(1000)
        elif command.name() == 'current_flux':
            time = float(command.kwargs.get('time', -1))
            flux = float(command.kwargs.get('flux', 0.0))

            self.fluxPlot.setText("time = " + str(time) + "\t flux=" + str(flux))
            self.fluxPlot.append(time, flux)

        elif command.name() == 'accumulated_flux':
            time = float(command.kwargs.get('time', -1))
            flux = float(command.kwargs.get('flux', 0.0))

            self.totalPlot.setText("time = " + str(time) + "\t flux=" + str(flux))
            self.totalPlot.append(time, flux)

if __name__ == '__main__':
    import sys
    import signal
    import argparse

    signal.signal(signal.SIGINT, signal.SIG_DFL)

    app = QApplication(sys.argv)

    args = immp.parse(' '.join(sys.argv))

    print 'Connecting to ' + args.kwargs.get('host', 'localhost') + ':' + args.kwargs.get('port', '5555')

    gui = FastGUI()
    gui.show()

    gui.setHost(args.kwargs.get('host', 'rak'), int(args.kwargs.get('port', '5555')))
    gui.setAttribute(Qt.WA_DeleteOnClose)

    gui.connect(gui, SIGNAL("closed()"), app.quit)

    sys.exit(app.exec_())
