# pylvgl
Python bindings for the LVGL graphics library

This project aims at creating Python bindings (i.e. a Python module) for the [LVGL embedded GUI library](https://lvgl.io). It does so by providing a bindings generator, i.e. a Python script that generates the source code for the Python module, by inspecting the LVGL source code.

## Use-cases
LVGL is aimed at embedded platforms with low resources available. As such, it may seem counter-intuitive to use it in combination with a high-level programming language such as Python. However, there are several use cases:

* Use on embedded systems

  Embedded systems are getting more and more powerful, allowing the use of Python as a programming language, which can significantly reduce the programming effort required to create a graphical user interface. Since full-blown user interface libraries such as Qt or wxWidgets may still be too demanding for these platforms, LVGL combined with pylvgl may be a suitable solution.
  
* Prototyping on PC platform

  Even if Python is not used on the embedded platform, it can be used to prototype the embedded user interface.
  
## Installation
Optionally run `generate.py` to create the latest version of `lvglmodule.c`, but this is already included in the GitHub repository for convenience.

Run `setup.py install` to build and install pylvgl.
## Road-map
Already implemented:

* Parsing of LVGL source code
* Python bindings for all LVGL objects
* Python constants for all LVGL enum constants
* Support for styles
* Action callbacks

To be implemented:

* Destruction / deletion of items
* ...

## Limitations
* lvgl.Style.copy() allocates a new lv_style_t struct, which is never freed, since the management of lv_style_t structs is troublesome. LVGL keeps references to those style objects, and as such it cannot be determined when it would be safe to free that data. In the current implementation. To be safe, the allocated memory is never freed (and thus a memory leak is present)

* The bindings-generator currently assumes a 16 bit-per-pixel screen configuration. This is checked in the generated lvglmodule.c at compile-time.

## Developer info
`generate.py` is the Python script that does the parsing of the LVGL source code and generates `lvglmodule.c`. 
Parsing is done using `pycparser` which analyses the LVGL C-code and returns a list of functions and defines. These results are then 
applied to the `lvglmodule_template.c` file, resulting in the `lvglmodule.c` file.

The `setup.py` script will compile the file `lvglmodule.c` into an object file.
