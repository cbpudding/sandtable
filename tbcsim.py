#!/usr/bin/python

# MIT License

# Copyright (c) 2024 Alexander Hill

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import time
import turtle
import sys

# Turtle functions ported directly from the sandtable code. ~ahill

def trtlLoop(code, offset, times):
    block = offset + 2
    depth = 1
    length = 0
    temp = None

    while (depth > 0) and ((offset + length + 2) < len(code)):
        temp = code[block + length]
        if (temp & 0xC0) == 0xC0:
            if (temp & 0xFF) == 0xC0:
                depth -= 1
            length += 1
        else:
            if (temp & 0xC0) == 0x80:
                depth += 1
            length += 2

    if (offset + length) >= len(code):
        return 0

    for i in range(times):
        trtlRun(code[block:(block + length)])

    return length + 2

def trtlAtomic(sub):
    if sub == 1:
        turtle.goto(0, 0)

def trtlExec(code, offset):
    data = None
    temp = code[offset]

    if (temp & 0xC0) == 0xC0:
        if (offset + 1) > len(code):
            return 0
    elif (offset + 2) > len(code):
        return 0
    
    if (temp & 0xC0) != 0xC0:
        data = ((temp & 0x3F) << 8) | (code[offset + 1] & 0xFF)
        if (data & 0x2000) and ((temp & 0xC0) != 0x80):
            data |= -1 & ~0x3FFF
    else:
        data = temp & 0x3F

    match (temp & 0xC0) >> 6:
        # Forward
        case 0b00:
            turtle.forward(data / 10)
            return 2
        # Left
        case 0b01:
            turtle.left(data)
            return 2
        # Loop
        case 0b10:
            return trtlLoop(code, offset, data)
        # Atomic
        case 0b11:
            trtlAtomic(temp & 0x3F)
            return 1
        case _:
            return 0

def trtlRun(code):
    offset = 0
    result = 1
    while (result != 0) and (offset < len(code)):
        result = trtlExec(code, offset)
        offset += result

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <pattern file>")
        exit(1)
    with open(sys.argv[1], "rb") as pattern:
        bytecode = pattern.read()
        marble = turtle.Turtle(visible = None)
        turtle.speed(10)
        trtlRun(bytecode)
        time.sleep(5)