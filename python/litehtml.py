#!/usr/bin/env vpython3
import sys
import wx
from ctypes import *

class CreateFont(Structure):
    _fields_ = (
        ('face', c_char_p),
        ('size', c_int),
        ('weight', c_int),
        ('italic', c_int),
        ('decor', c_uint),
        # out
        ('ascent', c_int),
        ('descent', c_int),
        ('height', c_int),
        ('xheight', c_int),
        ('font', c_int),
    )

class TextWidth(Structure):
    _fields_ = (
        ('text', c_char_p),
        ('font', c_int),
        # out
        ('width', c_int),
    )

class DrawText(Structure):
    _fields_ = (
        ('dc', c_int),
        ('text', c_char_p),
        ('font', c_int),
        ('color', c_uint),
        ('x', c_int),
        ('y', c_int),
    )

class DrawBackground(Structure):
    _fields_ = (
        ('dc', c_int),
        ('x', c_int),
        ('y', c_int),
        ('w', c_int),
        ('h', c_int),
        ('color', c_uint),
    )

class DrawBorders(Structure):
    _fields_ = (
        ('dc', c_int),
        ('left', c_int),
        ('right', c_int),
        ('top', c_int),
        ('bottom', c_int),
        ('colorLeft', c_uint),
        ('colorRight', c_uint),
        ('colorTop', c_uint),
        ('colorBottom', c_uint),
        ('widthLeft', c_uint),
        ('widthRight', c_uint),
        ('widthTop', c_uint),
        ('widthBottom', c_uint),
    )

class DrawMarker(Structure):
    _fields_ = (
        ('dc', c_int),
        ('x', c_int),
        ('y', c_int),
        ('w', c_int),
        ('h', c_int),
        ('mt', c_int),
        ('color', c_uint),
    )

class PT2PX(Structure):
    _fields_ = (
        ('pt', c_int),
    )

class LiteHtml:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pylib = CDLL("./py_container.so")
        self.pycb = CFUNCTYPE(None, c_char_p, POINTER(None))(self.cbproc)
        self.pyclass = self.pylib.container_create(width, height, self.pycb)

    def reset(self):
        self.bmp = wx.Bitmap(self.width, self.height, 32)
        self.dc = wx.MemoryDC()
        self.dc.SelectObject(self.bmp)
        self.fonts = {}
        self.ppi = self.dc.GetPPI()

    def close(self):
        self.pylib.container_delete(self.pyclass)

    def GetColourFromRGBA(self, rgba):
        a = rgba & 0xff; rgba >>= 8
        b = rgba & 0xff; rgba >>= 8
        g = rgba & 0xff; rgba >>= 8
        r = rgba & 0xff; rgba >>= 8
        return wx.Colour(r, g, b, a)

    def cbproc(self, name, argp):
        name = name.decode('utf8')
        getattr(self, name)(argp)

    def createFont(self, argp):
        pfi = cast(argp, POINTER(CreateFont))
        fi = pfi.contents
        face = fi.face
        if not face:
            face = 'Times New Roman'
        else:
            face = face.split(b',')[0].strip()
            if face[0] == b'"':
                face = face.split(b'"')[1]
            elif face[0] == b"'":
                face = face.split(b"'")[1]
            face = face.decode('utf8')
        if fi.italic:
            style = wx.FONTSTYLE_ITALIC
        else:
            style = wx.FONTSTYLE_NORMAL
        weigths = {
            100:wx.FONTWEIGHT_THIN,
            200:wx.FONTWEIGHT_EXTRALIGHT,
            300:wx.FONTWEIGHT_LIGHT,
            400:wx.FONTWEIGHT_NORMAL,
            500:wx.FONTWEIGHT_MEDIUM,
            600:wx.FONTWEIGHT_SEMIBOLD,
            700:wx.FONTWEIGHT_BOLD,
            800:wx.FONTWEIGHT_EXTRABOLD,
            900:wx.FONTWEIGHT_HEAVY,
            1000:wx.FONTWEIGHT_EXTRAHEAVY,
        }
        weight = weigths.get(fi.weight, wx.FONTWEIGHT_NORMAL)
        underline = fi.decor != 0
        font = wx.Font(fi.size, wx.FONTFAMILY_DEFAULT, style, weight, underline, face)
        nfont = len(self.fonts)
        self.fonts[nfont] = font
        if 0:
            w, h, d, e = self.dc.GetFullTextExtent('H', font)
            xw, xh, xd, xe = self.dc.GetFullTextExtent('x', font)
            a = 0
        else:
            self.dc.SetFont(font)
            fm = self.dc.GetFontMetrics()
            a = fm.ascent
            d = fm.descent
            h = fm.height
            xh = h
        fi.ascent = a
        fi.descent = d
        fi.height = h
        fi.xheight = xh
        fi.font = nfont

    def textWidth(self, argp):
        pfi = cast(argp, POINTER(TextWidth))
        fi = pfi.contents

        font = self.fonts[fi.font]
        text = fi.text.decode('utf8')
        w, h, d, e = self.dc.GetFullTextExtent(text, font)
        fi.width = w

    def drawText(self, argp):
        pfi = cast(argp, POINTER(DrawText))
        fi = pfi.contents

        font = self.fonts[fi.font]
        text = fi.text.decode('utf8')
        color = self.GetColourFromRGBA(fi.color)
        x = fi.x
        y = fi.y

        self.dc.SetFont(font)
        self.dc.SetTextForeground(color)
        self.dc.DrawText(text, x, y)

    def drawBackground(self, argp):
        pfi = cast(argp, POINTER(DrawBackground))
        fi = pfi.contents

        x = fi.x
        y = fi.y
        w = fi.w
        h = fi.h
        color = self.GetColourFromRGBA(fi.color)
        pt = [
            (x, y), (x+w, y), (x+w, y+h), (x, y+h), (x, y)
        ]
        self.dc.SetBrush(wx.Brush(color))
        self.dc.DrawPolygon(pt)
        self.dc.SetBrush(wx.NullBrush)

    def drawMarker(self, argp):
        pfi = cast(argp, POINTER(DrawMarker))
        fi = pfi.contents

        x = fi.x
        y = fi.y
        w = fi.w
        h = fi.h
        mt = fi.mt
        color = self.GetColourFromRGBA(fi.color)
        print('drawMarker', x, y, w, h)

    def pt2px(self, argp):
        pfi = cast(argp, POINTER(PT2PX))
        fi = pfi.contents

        pt = fi.pt
        fi.pt = int(pt * self.ppi[0] / 72)

    def drawBorders(self, argp):
        pfi = cast(argp, POINTER(DrawBorders))
        fi = pfi.contents

        left = fi.left
        right = fi.right
        top = fi.top
        bottom = fi.bottom
        colorLeft = self.GetColourFromRGBA(fi.colorLeft)
        colorRight = self.GetColourFromRGBA(fi.colorRight)
        colorTop = self.GetColourFromRGBA(fi.colorTop)
        colorBottom = self.GetColourFromRGBA(fi.colorBottom)
        widthLeft = fi.widthLeft
        widthRight = fi.widthRight
        widthTop = fi.widthTop
        widthBottom = fi.widthBottom

        self.dc.SetPen(wx.Pen(colorLeft, widthLeft))
        self.dc.DrawLine(left, top, left, bottom)
        self.dc.SetPen(wx.Pen(colorTop, widthTop))
        self.dc.DrawLine(left, top, right, top)
        self.dc.SetPen(wx.Pen(colorRight, widthRight))
        self.dc.DrawLine(right, top, right, bottom)
        self.dc.SetPen(wx.Pen(colorBottom, widthBottom))
        self.dc.DrawLine(left, bottom, right, bottom)

    def render(self, fname):
        self.reset()
        html = open(fname, 'rb').read()
        self.pylib.container_render(self.pyclass, html)
        #bmp = self.dc.GetSelectedBitmap()
        self.bmp.SaveFile(fname+'.png', wx.BITMAP_TYPE_PNG)

def main():
    wxapp = wx.App(False)
    v = 3.96* 96 / 72
    print('v=', v)
    w = int(210 * v)
    h = int(297 * v)
    #w = 1600
    cls = LiteHtml(w, h)
    if len(sys.argv) > 1:
        cls.render(sys.argv[1])
    elif 1:
        for i in (0, 1, 2, 3):
            cls.render("pit-11-29-%d.html"%i)
    else:
        cls.render("pit-11-29.html")
    cls.close()

if __name__ == '__main__':
    main()
