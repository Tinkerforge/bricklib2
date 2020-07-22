#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import colorsys

intensity = 0.75
num = 250

for i in reversed(range(num)):
    r, g, b = colorsys.hsv_to_rgb(1.0*i/(num-1), 1, intensity)
    print("{{{0}, {1}, {2}}},".format(int(r*255),int(g*255), int(b*255)))
