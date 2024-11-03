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

import os
import re
import sys

def hcf(number, message):
    print(f"Error on line {number}: {message}")
    exit(1)

def atom(outfile, number, subroutine):
    if subroutine < 0 or subroutine > 63:
        hcf(number, f"Atom {subroutine} out of range")
    outfile.write(bytes([0xC0 | subroutine]))

def forward(outfile, number, units):
    if units < -8192 or units > 8191:
        hcf(number, "Units cannot be greater than 8191 or less than -8192")
    outfile.write(bytes([(units & 0x3F00) >> 8, units & 0xFF]))

def left(outfile, number, degrees):
    if degrees < -8192 or degrees > 8191:
        hcf(number, "Why would you want to rotate more than 359 degrees anyways?")
    outfile.write(bytes([0x40 | ((degrees & 0x3F00) >> 8), degrees & 0xFF]))

def loop(outfile, number, times):
    if times < 0 or times > 16383:
        hcf(number, "Cannot loop more than 16383 times")
    outfile.write(bytes([0x80 | ((times & 0x3F00) >> 8), times & 0xFF]))

def compile(input, output = "turtle.bin"):
    with open(input) as infile:
        with open(output, "wb") as outfile:
            code = infile.readlines()
            depth = 0
            for number, line in enumerate(code):
                tokens = line.strip().split()
                if len(tokens) > 0 and not tokens[0].startswith("#"):
                    match tokens[0]:
                        case "atom":
                            if len(tokens) == 2:
                                atom(outfile, number, int(tokens[1]))
                            else:
                                hcf(number, "Must specify an atom ID")
                        case "backward":
                            if len(tokens) == 2:
                                forward(outfile, number, -int(tokens[1]))
                            else:
                                hcf(number, "Must specify how many units to move")
                        case "center":
                            atom(outfile, number, 1)
                        case "endloop":
                            atom(outfile, number, 0)
                            depth -= 1
                        case "forward":
                            if len(tokens) == 2:
                                forward(outfile, number, int(tokens[1]))
                            else:
                                hcf(number, "Must specify how many units to move")
                        case "left":
                            if len(tokens) == 2:
                                left(outfile, number, int(tokens[1]))
                            else:
                                hcf(number, "Must specify how many degrees to rotate")
                        case "loop":
                            if len(tokens) == 2:
                                loop(outfile, number, int(tokens[1]))
                                depth += 1
                            else:
                                hcf(number, "Must specify the number of times to loop")
                        case "right":
                            if len(tokens) == 2:
                                left(outfile, number, -int(tokens[1]))
                            else:
                                hcf(number, "Must specify how many degrees to rotate")
                        case _:
                            hcf(number, f"Invalid instruction: {tokens[0]}")
            if depth > 0:
                hcf(len(code), "Loop without end found")
            elif depth < 0:
                hcf(len(code), "End without loop found")

def dump(input, output = None):
    with open(input, "rb") as infile:
        if output and output != "-":
            outfile = open(output, "w")
        else:
            outfile = sys.stdout
        with outfile:
            constname = re.sub("[^A-Z0-9]", "_", os.path.basename(input.upper()))
            pattern = infile.read()
            array = ", ".join(f"0x{byte:02X}" for byte in pattern)
            outfile.write(f"const char {constname}[] = {{ {array} }};")

def usage():
    print(f"Usage: {sys.argv[0]} <compile|dump> <input file> [output file]")
    exit(1)

if __name__ == "__main__":
    if len(sys.argv) >= 3:
        if sys.argv[1] == "compile":
            if len(sys.argv) >= 4:
                compile(sys.argv[2], sys.argv[3])
            else:
                compile(sys.argv[2])
        elif sys.argv[1] == "dump":
            if len(sys.argv) >= 4:
                dump(sys.argv[2], sys.argv[3])
            else:
                dump(sys.argv[2])
        else:
            usage()
    else:
        usage()