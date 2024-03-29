#!/usr/bin/env python3

from twisted.internet.protocol import Protocol, ReconnectingClientFactory
from twisted.internet.task import LoopingCall
from twisted.web.server import Site
from twisted.web.resource import Resource
from twisted.web.static import File

# from twistedauth import wrap_with_auth as Auth

from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.dates import DateFormatter
from matplotlib.ticker import ScalarFormatter
from matplotlib.patches import Ellipse

import sys
from urllib.parse import urlparse, parse_qs
import json
import datetime
import re

from PIL import Image, ImageDraw
from io import BytesIO, StringIO

import matplotlib.cm as cm

import numpy as np

import immp

class Fast:
    def __init__(self):
        self.fast_status = {}
        self.image = None
        self.total_image = None
        self.time = []
        self.flux = []
        self.mean = []
        self.status_counter = 0

# Socket part

class Favor2Protocol(Protocol):
    refresh = 1

    def connectionMade(self):
        self.buffer = b''
        self.is_binary = False
        self.binary_length = 0

        self.factory.clients.append(self)

        LoopingCall(self.requestStatus).start(self.refresh)

    def connectionLost(self, reason):
        self.factory.clients.remove(self)

    def message(self, string):
        #print ">>", string

        if type(string) == str:
            self.transport.write(string.encode('ascii'))
        else:
            self.transport.write(string)

        self.transport.write("\0".encode('ascii'))

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
                    token, self.buffer = re.split(b'\0|\n', self.buffer, 1)
                    if token and len(token):
                        self.processMessage(token.decode('ascii'))
                except ValueError:
                    break

    def processMessage(self, string):
        print(">", string)

    def processBinary(self, data):
        print("binary>", len(data), "bytes")

    def requestStatus(self):
        pass

class FastProtocol(Favor2Protocol):
    def connectionMade(self):
        self.factory.object.image = None
        self.factory.object.total_image = None

        self.factory.object.time = []
        self.factory.object.flux = []
        self.factory.object.mean = []

        self.factory.object.status_counter = 0

        self.wait_current = False
        self.wait_total = False

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
        if not self.wait_current:
            self.wait_current = True
            self.message("get_current_frame")
        #if self.factory.object.status_counter == 0:
        if not self.wait_total:
            self.wait_total = True
            self.message("get_total_frame")

        self.factory.object.status_counter += 1
        if self.factory.object.status_counter > 10:
            self.factory.object.status_counter = 0

    def processMessage(self, string):
        command = immp.parse(string)

        if command.name() == 'current_frame':
            self.wait_current = False
            self.is_binary = True
            self.binary_length = int(command.kwargs.get('length', 0))
            self.image_format = command.kwargs.get('format', '')
            if self.factory.object.fast_status:
                self.factory.object.fast_status['image_mean'] = float(command.kwargs.get('mean', 0))
            self.image_type = 'current'

        elif command.name() == 'current_frame_timeout':
            self.wait_current = False

        elif command.name() == 'total_frame':
            self.wait_total = False
            self.is_binary = True
            self.binary_length = int(command.kwargs.get('length', 0))
            self.image_format = command.kwargs.get('format', '')
            self.image_type = 'total'

        elif command.name() == 'total_frame_timeout':
            self.wait_total = False

        elif command.name() == 'fast_status' or command.name() == 'status':
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

class Favor2ClientFactory(ReconnectingClientFactory):
#    factor = 1 # Disable exponential growth of reconnection delay
    maxDelay = 2

    def __init__(self, factory, object=None):
        self.object = object
        self.protocolFactory = factory
        self.clients = []

    def buildProtocol(self, addr):
        print('Connected to %s:%d' % (addr.host, addr.port))

        p = self.protocolFactory()
        p.factory = self

        return p

    def clientConnectionLost(self, connector, reason):
        print('Disconnected from %s:%d' % (connector.host, connector.port))
        ReconnectingClientFactory.clientConnectionLost(self, connector, reason)

    def isConnected(self):
        return len(self.clients) > 0

    def message(self, string):
        for c in self.clients:
            c.message(string)

# Web part

def serve_json(request, **kwargs):
    request.responseHeaders.setRawHeaders("Content-Type", ['application/json'])
    return json.dumps(kwargs).encode('ascii')

class WebPhotometer(Resource):
    isLeaf = True

    def __init__(self, photometer):
        self.photometer = photometer

    def render_GET(self, request):
        q = urlparse(request.uri)
        args = parse_qs(q.query)

        if q.path == '/photometer/status':
            return serve_json(request,
                              photometer_connected = self.photometer.factory.isConnected(),
                              photometer = self.photometer.photometer_status)
        elif q.path == '/photometer/command':
            self.photometer.factory.message(args[b'string'][0])
            return serve_json(request)
        else:
            return q.path

class WebFast(Resource):
    isLeaf = True

    def __init__(self, fast, base='/fast'):
        self.fast = fast
        self.base = base

    def render_GET(self, request):
        q = urlparse(request.uri)
        args = parse_qs(q.query)
        path = q.path.decode('ascii')

        if path == self.base + '/status':
            return serve_json(request,
                              fast_connected = self.fast.factory.isConnected(),
                              fast = self.fast.fast_status)
        elif path == self.base + '/command':
            self.fast.factory.message(args[b'string'][0])
            return serve_json(request)
        elif path == self.base + '/image.jpg':
            if self.fast.image:
                request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
                request.responseHeaders.setRawHeaders("Content-Length", [str(len(self.fast.image))])
                return self.fast.image
            else:
                request.setResponseCode(400)
                return "No images".encode('ascii')
        elif path == self.base + '/total_image.jpg':
            if self.fast.total_image:
                request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
                request.responseHeaders.setRawHeaders("Content-Length", [str(len(self.fast.total_image))])
                return self.fast.total_image
            else:
                request.setResponseCode(400)
                return "No images".encode('ascii')
        elif path == self.base + '/total_flux.jpg':
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
            s = BytesIO()
            canvas.print_jpeg(s)

            request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
            request.responseHeaders.setRawHeaders("Content-Length", [str(len(s.getvalue()))])
            request.responseHeaders.setRawHeaders("Cache-Control", ['no-store, no-cache, must-revalidate, max-age=0'])
            return s.getvalue()

        elif path == self.base + '/current_flux.jpg':
            width = 800
            fig = Figure(facecolor='white', figsize=(width/72, width*0.5/72), tight_layout=True)
            ax = fig.add_subplot(111)

            ax.yaxis.set_major_formatter(ScalarFormatter(useOffset=False))

            x = np.array(self.fast.time)
            if len(x):
                x = x - x[0]
            y = np.array(self.fast.mean)

            ax.plot(x[-1000:], y[-1000:], '-')
            ax.set_xlabel('Time, seconds')
            ax.set_ylabel('Flux, counts')

            canvas = FigureCanvas(fig)
            s = BytesIO()
            canvas.print_jpeg(s)

            request.responseHeaders.setRawHeaders("Content-Type", ['image/jpeg'])
            request.responseHeaders.setRawHeaders("Content-Length", [str(len(s.getvalue()))])
            request.responseHeaders.setRawHeaders("Cache-Control", ['no-store, no-cache, must-revalidate, max-age=0'])
            return s.getvalue()

        else:
            return path.encode('ascii')

# Main
if __name__ == '__main__':
    from optparse import OptionParser

    parser = OptionParser(usage="usage: %prog [options] ...")
    parser.add_option('-p', '--port', help='Daemon port', action='store', dest='port', type='int', default=5555)
    parser.add_option('-H', '--http-port', help='HTTP server port', action='store', dest='http_port', type='int', default=5556)

    (options,args) = parser.parse_args()

    fast = Fast()

    fast.factory = Favor2ClientFactory(FastProtocol, fast)

    from twisted.internet import reactor
    reactor.connectTCP('localhost', options.port, fast.factory)

    passwd = {'fast':'fast'}

    # Serve files from web
    root = File(b"web")
    # ...with some other paths handled also
    root.putChild(b"", File('web/webfast.html'))
    root.putChild(b"fast.html", File('web/webfast.html'))

    root.putChild(b"fast", WebFast(fast, base='/fast'))

    #site = Site(Auth(root, passwd))
    site = Site(root)

    reactor.listenTCP(options.http_port, site)

    reactor.run()
