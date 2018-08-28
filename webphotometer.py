#!/usr/bin/env python

from twisted.internet.protocol import Protocol, ReconnectingClientFactory
from twisted.internet.task import LoopingCall
from twisted.web.server import Site
from twisted.web.resource import Resource
from twisted.web.static import File

from twistedauth import wrap_with_auth as Auth

from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.dates import DateFormatter
from matplotlib.ticker import ScalarFormatter
from matplotlib.patches import Ellipse

import sys
import urlparse
import json
import datetime
import re

from PIL import Image, ImageDraw
from StringIO import StringIO

import matplotlib.cm as cm

import numpy as np

import immp

class Photometer:
    def __init__(self):
        self.photometer_status = {}
        self.hw_status = {}
        self.image = None

class Fast:
    def __init__(self):
        self.fast_status = {}
        self.image = None
        self.total_image = None
        self.time = []
        self.flux = []
        self.mean = []
        self.status_counter = 0

class OldGuider:
    def __init__(self):
        self.fast_status = {}
        self.acs_status = {}
        self.acquisition = True
        self.exposure = 0.4
        self.image = None

# Socket part

class Favor2Protocol(Protocol):
    refresh = 1

    def connectionMade(self):
        self.buffer = ''
        self.is_binary = False
        self.binary_length = 0

        self.factory.clients.append(self)

        LoopingCall(self.requestStatus).start(self.refresh)

    def connectionLost(self, reason):
        self.factory.clients.remove(self)

    def message(self, string):
        #print ">>", string
        self.transport.write(string)
        self.transport.write("\0")

    def dataReceived(self, data):
        self.buffer = self.buffer + data

        while len(self.buffer):
            if self.is_binary:
                if len(self.buffer) >= self.binary_length:
                    bdata = self.buffer[:self.binary_length]
                    self.buffer = self.buffer[self.binary_length:]

                    self.processBinary(bdata)
                    self.is_binary = False
                else:
                    break
            else:
                try:
                    #token, self.buffer = self.buffer.split('\0', 1)
                    token, self.buffer = re.split('\0|\n', self.buffer, 1)
                    if token and len(token):
                        self.processMessage(token)
                except ValueError:
                    break

    def processMessage(self, string):
        print ">", string

    def processBinary(self, data):
        print "binary>", len(data), "bytes"

    def requestStatus(self):
        pass

class PhotometerProtocol(Favor2Protocol):
    def connectionMade(self):
        Favor2Protocol.connectionMade(self)

    def connectionLost(self, reason):
        Favor2Protocol.connectionLost(self, reason)

    def requestStatus(self):
        self.message("get_status")

    def processMessage(self, string):
        command = immp.parse(string)

        if command.name() == 'photometer_status':
            self.factory.object.photometer_status = command.kwargs

class FastProtocol(Favor2Protocol):
    def connectionMade(self):
        self.factory.object.image = None
        self.factory.object.total_image = None

        self.factory.object.time = []
        self.factory.object.flux = []
        self.factory.object.mean = []

        self.factory.object.status_counter = 0

        Favor2Protocol.connectionMade(self)

    def connectionLost(self, reason):
        self.factory.object.image = None
        self.factory.object.total_image = None

        self.factory.object.time = []
        self.factory.object.flux = []
        self.factory.object.mean = []

        Favor2Protocol.connectionLost(self, reason)

    def requestStatus(self):
        self.message("get_status")
        self.message("get_current_frame")
        #if self.factory.object.status_counter == 0:
        self.message("get_total_frame")

        self.factory.object.status_counter += 1
        if self.factory.object.status_counter > 10:
            self.factory.object.status_counter = 0

    def processMessage(self, string):
        command = immp.parse(string)

        if command.name() == 'current_frame':
            self.is_binary = True
            self.binary_length = int(command.kwargs.get('length', 0))
            self.image_format = command.kwargs.get('format', '')
            if self.factory.object.fast_status:
                self.factory.object.fast_status['image_mean'] = float(command.kwargs.get('mean', 0))
            self.image_type = 'current'

        elif command.name() == 'total_frame':
            self.is_binary = True
            self.binary_length = int(command.kwargs.get('length', 0))
            self.image_format = command.kwargs.get('format', '')
            self.image_type = 'total'

        elif command.name() == 'fast_status':
            self.factory.object.fast_status = command.kwargs
            if command.kwargs.get('acquisition') == '0':
                self.factory.object.time = []
                self.factory.object.flux = []
                self.factory.object.mean = []

        elif command.name() == 'current_flux':
            if self.factory.object.time and len(self.factory.object.time) > 100000:
                self.factory.object.time = []
                self.factory.object.flux = []
                self.factory.object.mean = []

            self.factory.object.time.append(float(command.kwargs.get('time', 0)))
            self.factory.object.flux.append(float(command.kwargs.get('flux', 0)))
            self.factory.object.mean.append(float(command.kwargs.get('mean', 0)))

    def processBinary(self, data):
        if self.image_type == 'current':
            self.factory.object.image = data
            self.factory.object.image_format = self.image_format
        elif self.image_type == 'total':
            self.factory.object.total_image = data

class OldGuiderProtocol(Favor2Protocol):
    def connectionMade(self):
        self.wait_image = False
        self.wait_image_time = datetime.datetime.now()

        self.factory.object.acquisition = False
        self.factory.object.exposure = 0.4

        self.factory.object.image = None
        Favor2Protocol.connectionMade(self)

    def connectionLost(self, reason):
        self.factory.object.image = None
        Favor2Protocol.connectionLost(self, reason)

    def requestStatus(self):
        if not self.factory.object.acquisition:
            return

        if not self.wait_image or (datetime.datetime.now() - self.wait_image_time).total_seconds() > 30:
            self.message("get_image %d" % (int(self.factory.object.exposure/0.04)))
            self.wait_image = True
            self.wait_image_time = datetime.datetime.now()

    def processMessage(self, string):
        command = string.split()

        if command[0] == 'image':
            self.is_binary = True
            self.width = int(command[1])
            self.height = int(command[2])
            self.binary_length = self.width*self.height

    def processBinary(self, data):
        arr = np.fromstring(data, dtype=np.uint8).reshape(self.height, self.width)

        vmin,vmax = np.percentile(arr, [0.5,95.5])
        if vmax < 10:
            vmax = 10

        #arr[arr == np.max(arr)] = 0

        arr = 1.0*(arr - vmin)/(vmax-vmin)
        arr[arr<0] = 0
        arr[arr>1] = 1

        cm_hot = cm.get_cmap('hot')
        arr = cm_hot(arr)*255

        image = Image.fromarray(np.uint8(arr))

        draw = ImageDraw.Draw(image)
        #draw.line([0,0,100,100], fill='blue', width=3)
        x0,y0 = 377,228
        draw.ellipse([x0-10,y0-10,x0+10,y0+10], outline='green')

        if self.factory.object.acs_status.has_key('P2'):
            x0, y0 = image.size[0] - 100, 100
            r0 = 70
            positionAngle = 50.7
            alpha = np.deg2rad(-float(self.factory.object.acs_status.get('P2', 0)) - positionAngle)

            # A
            draw.line([x0, y0, x0 + r0*np.sin(alpha), y0 + r0*np.cos(alpha)], fill='green', width=2)
            draw.line([x0 + r0*np.sin(alpha), y0 + r0*np.cos(alpha), x0 + 0.8*r0*np.sin(alpha+0.05), y0 + 0.8*r0*np.cos(alpha+0.05)], fill='green', width=2)
            draw.line([x0 + r0*np.sin(alpha), y0 + r0*np.cos(alpha), x0 + 0.8*r0*np.sin(alpha-0.05), y0 + 0.8*r0*np.cos(alpha-0.05)], fill='green', width=2)

            draw.text([x0 + 1.2*r0*np.sin(alpha), y0 + 1.2*r0*np.cos(alpha)], "A", fill='green')
            draw.text([x0 + 1.2*r0*np.cos(alpha), y0 - 1.2*r0*np.sin(alpha)], "Z", fill='green')

            # Z
            draw.line([x0, y0, x0+r0*np.cos(alpha), y0-r0*np.sin(alpha)], fill='green', width=2)

        str = StringIO()
        image.save(str, 'jpeg', quality=95)

        self.factory.object.image = str.getvalue()

        self.wait_image = False

class AcsProtocol(Favor2Protocol):
    def connectionMade(self):
        Favor2Protocol.connectionMade(self)

    def connectionLost(self, reason):
        Favor2Protocol.connectionLost(self, reason)

    def requestStatus(self):
        self.message("Get Alpha Delta AzCur ZdCur P2 PA Mtime Stime")

    def processMessage(self, string):
        command = immp.parse(string)

        if command.name() == 'none':
            self.factory.object.acs_status = command.kwargs

class Favor2ClientFactory(ReconnectingClientFactory):
#    factor = 1 # Disable exponential growth of reconnection delay
    maxDelay = 2

    def __init__(self, factory, object=None):
        self.object = object
        self.protocolFactory = factory
        self.clients = []

    def buildProtocol(self, addr):
        print 'Connected to %s:%d' % (addr.host, addr.port)

        p = self.protocolFactory()
        p.factory = self

        return p

    def clientConnectionLost(self, connector, reason):
        print 'Disconnected from %s:%d' % (connector.host, connector.port)
        ReconnectingClientFactory.clientConnectionLost(self, connector, reason)

    def isConnected(self):
        return len(self.clients) > 0

    def message(self, string):
        for c in self.clients:
            c.message(string)

# Web part

def serve_json(request, **kwargs):
    request.responseHeaders.setRawHeaders("Content-Type", ['application/json'])
    return json.dumps(kwargs)

class WebPhotometer(Resource):
    isLeaf = True

    def __init__(self, photometer):
        self.photometer = photometer

    def render_GET(self, request):
        q = urlparse.urlparse(request.uri)
        args = urlparse.parse_qs(q.query)

        if q.path == '/photometer/status':
            return serve_json(request,
                              photometer_connected = self.photometer.factory.isConnected(),
                              photometer = self.photometer.photometer_status)
        elif q.path == '/photometer/command':
            self.photometer.factory.message(args['string'][0])
            return serve_json(request)
        else:
            return q.path

class WebFast(Resource):
    isLeaf = True

    def __init__(self, fast, base='/fast'):
        self.fast = fast
        self.base = base

    def render_GET(self, request):
        q = urlparse.urlparse(request.uri)
        args = urlparse.parse_qs(q.query)

        if q.path == self.base + '/status':
            return serve_json(request,
                              fast_connected = self.fast.factory.isConnected(),
                              fast = self.fast.fast_status)
        elif q.path == self.base + '/command':
            self.fast.factory.message(args['string'][0])
            return serve_json(request)
        elif q.path == self.base + '/image.jpg':
            if self.fast.image:
                request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
                request.responseHeaders.setRawHeaders("Content-Length", [len(self.fast.image)])
                return self.fast.image
            else:
                request.setResponseCode(400)
                return "No images"
        elif q.path == self.base + '/total_image.jpg':
            if self.fast.total_image:
                request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
                request.responseHeaders.setRawHeaders("Content-Length", [len(self.fast.total_image)])
                return self.fast.total_image
            else:
                request.setResponseCode(400)
                return "No images"
        elif q.path == self.base + '/total_flux.jpg':
            width = 800
            fig = Figure(facecolor='white', figsize=(width/72, width*0.5/72), tight_layout=True)
            ax = fig.add_subplot(111)

            ax.yaxis.set_major_formatter(ScalarFormatter(useOffset=False))

            x = np.array(self.fast.time)
            x = x - x[0]

            ax.plot(x, self.fast.mean, '-')
            ax.set_xlabel('Time, seconds')
            ax.set_ylabel('Flux, counts')

            canvas = FigureCanvas(fig)
            s = StringIO()
            canvas.print_jpeg(s, bbox_inches='tight')

            request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
            request.responseHeaders.setRawHeaders("Content-Length", [s.len])
            request.responseHeaders.setRawHeaders("Cache-Control", ['no-store, no-cache, must-revalidate, max-age=0'])
            return s.getvalue()

        elif q.path == self.base + '/current_flux.jpg':
            width = 800
            fig = Figure(facecolor='white', figsize=(width/72, width*0.5/72), tight_layout=True)
            ax = fig.add_subplot(111)

            ax.yaxis.set_major_formatter(ScalarFormatter(useOffset=False))

            x = np.array(self.fast.time)
            x = x - x[0]
            y = np.array(self.fast.mean)

            ax.plot(x[-1000:], y[-1000:], '-')
            ax.set_xlabel('Time, seconds')
            ax.set_ylabel('Flux, counts')

            canvas = FigureCanvas(fig)
            s = StringIO()
            canvas.print_jpeg(s, bbox_inches='tight')

            request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
            request.responseHeaders.setRawHeaders("Content-Length", [s.len])
            request.responseHeaders.setRawHeaders("Cache-Control", ['no-store, no-cache, must-revalidate, max-age=0'])
            return s.getvalue()


        else:
            return q.path

class WebOldGuider(Resource):
    isLeaf = True

    def __init__(self, fast, base='/fast'):
        self.fast = fast
        self.base = base

    def render_GET(self, request):
        q = urlparse.urlparse(request.uri)
        args = urlparse.parse_qs(q.query)

        if q.path == self.base + '/status':
            status = {'acquisition':self.fast.acquisition, 'storage':0, 'vs56':1, 'exposure':self.fast.exposure}
            status.update(self.fast.acs_status)
            return serve_json(request,
                              fast_connected = self.fast.factory.isConnected(),
                              fast = status)
        elif q.path == self.base + '/command':
            command = immp.parse(args['string'][0])
            if command.name() == 'start':
                self.fast.acquisition = True
            elif command.name() == 'stop':
                self.fast.acquisition = False
            elif command.name() == 'set_grabber':
                if command.kwargs.has_key('exposure'):
                    self.fast.exposure = float(command.kwargs.get('exposure', 0.4))

            return serve_json(request)
        elif q.path == self.base + '/image.jpg':
            if self.fast.image:
                request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
                request.responseHeaders.setRawHeaders("Content-Length", [len(self.fast.image)])
                return self.fast.image
            else:
                request.setResponseCode(400)
                return "No images"
        else:
            return q.path

# Main
if __name__ == '__main__':
    photometer = Photometer()
    fast = Fast()
    guider = Fast()
    oldguider = OldGuider()

    photometer.factory = Favor2ClientFactory(PhotometerProtocol, photometer)
    fast.factory = Favor2ClientFactory(FastProtocol, fast)
    guider.factory = Favor2ClientFactory(FastProtocol, guider)
    oldguider.factory = Favor2ClientFactory(OldGuiderProtocol, oldguider)
    oldguider.acsfactory = Favor2ClientFactory(AcsProtocol, oldguider)

    from twisted.internet import reactor
    reactor.connectTCP('localhost', 5556, photometer.factory)
    reactor.connectTCP('rak.sao.ru', 5555, fast.factory)
    reactor.connectTCP('mmt2.sao.ru', 5555, guider.factory)
    reactor.connectTCP('abc.sao.ru', 5433, oldguider.factory)
    reactor.connectTCP('tb.sao.ru', 7654, oldguider.acsfactory)

    passwd = {'fast':'fast'}

    # Serve files from web
    root = File("web")
    # ...with some other paths handled also
    root.putChild("", File('web/webphotometer.html'))
    root.putChild("photometer.html", File('web/webphotometer.html'))
    root.putChild("fast.html", File('web/webphotometer.html'))
    root.putChild("guider.html", File('web/webphotometer.html'))
    root.putChild("oldguider.html", File('web/webphotometer.html'))
    root.putChild("mppp.html", File('web/webphotometer.html'))

    root.putChild("photometer", WebPhotometer(photometer))
    root.putChild("fast", WebFast(fast, base='/fast'))
    root.putChild("guider", WebFast(guider, base='/guider'))
    root.putChild("oldguider", WebOldGuider(oldguider, base='/oldguider'))

    #site = Site(Auth(root, passwd))
    site = Site(root)

    reactor.listenTCP(8888, site)

    reactor.run()
