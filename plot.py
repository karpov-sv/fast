from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from PyQt4.Qwt5 import *
from PyQt4 import uic

import random
import math

class Plot(QWidget):
    def __init__(self):
        super(Plot, self).__init__()

        self.ui = uic.loadUi("gui/plot.ui", self)

        self.ui.plot.setCanvasBackground(Qt.white)
        self.ui.plot.setAxisTitle(QwtPlot.xBottom, 'Time')
        self.ui.plot.setAxisAutoScale(QwtPlot.xBottom)
        self.ui.plot.setAxisTitle(QwtPlot.yLeft, 'Flux')
        self.ui.plot.setAxisAutoScale(QwtPlot.yLeft)
        self.ui.plot.replot()

        self.curve = Qwt.QwtPlotCurve('')
        self.curve.setRenderHint(QwtPlotItem.RenderAntialiased)

        pen = QPen(QColor('blue'))
        pen.setWidth(1)
        self.curve.setPen(pen)
        self.curve.setStyle(QwtPlotCurve.Steps)
        self.curve.attach(self.ui.plot)

        self.zoomer = QwtPlotZoomer(QwtPlot.xBottom, QwtPlot.yLeft,
                                    QwtPicker.DragSelection, QwtPicker.AlwaysOn, self.ui.plot.canvas())

        self.connect(self.zoomer, SIGNAL("zoomed"), lambda rect: self.replot())

        self.data = []
        self.maxLength = 0

        self.x = 0

    def closeEvent(self, event):
        self.emit(SIGNAL("closed()"))
        QWidget.closeEvent(self, event)

    def append(self, x, y):
        # Reset the plot if the data is in the past
        if len(self.data) and x < self.data[-1][0]:
            self.data = []

        self.data.append((x, y))

        if self.maxLength and len(self.data) > self.maxLength:
            self.data.pop(0)

        if not self.zoomer.zoomRectIndex():
            self.replot()

    def replot(self):
        xdata = [i[0] for i in self.data]
        ydata = [i[1] for i in self.data]

        self.ui.plot.setAxisScale(QwtPlot.xBottom, xdata[0], xdata[-1])
        self.ui.plot.setAxisAutoScale(QwtPlot.yLeft)
        self.curve.setData(xdata, ydata)
        self.ui.plot.replot()
        self.zoomer.setZoomBase(False)

    def setText(self, text):
        self.ui.label.setText(text)

    def addRandom(self):
        self.append(self.x, random.random()*math.sin(1.0*self.x/100))
        self.x = self.x + 1

# Main
if __name__ == '__main__':
    import sys
    import signal

    signal.signal(signal.SIGINT, signal.SIG_DFL)

    app = QApplication(sys.argv)

    plot = Plot()
    plot.show()
    plot.maxLength = 100

    plot.connect(plot, SIGNAL("closed()"), app.quit)

    timer = QTimer()
    timer.start(100)

    plot.connect(timer, SIGNAL("timeout()"), plot.addRandom)

    sys.exit(app.exec_())
