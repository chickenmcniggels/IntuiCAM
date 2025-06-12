# Details regarding the CAM-Core-Engine
## Turning Toolpath Generation
**Default turning toolpaths**
1. **Contouring** – combines facing, roughing and finishing passes.
2. **Threading** – configurable pitch and face selection.
3. **Chamfering** – configurable edge selection and size.
4. **Parting** – defines how the part is cut off.

Additional specialized operations such as grooving or drilling may be added in the future.

All turning need to support cam-side cutter radius compensation. This can also be done on the machine-side therefore it must be toggleable.

## Milling Toolpath Generation
**The following milling toolpaths are proposed:**
* Basic Linear Roughing
* Adaptive Roughing
* Contouring
* Drilling
* Chamfer Milling
* Radius Milling
* (Thread Milling)

