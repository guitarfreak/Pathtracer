
## Simple Path Tracer

<p float="right">
	<a href="https://guitarfreak.github.io/assets/images/pathtracer1.png">
  		<img src="https://guitarfreak.github.io/assets/images/pathtracer1.png" width="49%" />
  	</a>
	<a href="https://guitarfreak.github.io/assets/images/pathtracer2.png">
		<img src="https://guitarfreak.github.io/assets/images/pathtracer2.png" width="49%" />
  	</a>
</p>

<a href="https://github.com/guitarfreak/PropertyWatcher/assets/1862206/dbd22884-693a-4e99-a08c-158c970683b3">
	<img src="https://github.com/guitarfreak/PropertyWatcher/assets/1862206/dbd22884-693a-4e99-a08c-158c970683b3" width="49%" />
</a>

#### A path tracer with basic functionality

* Reflection, refraction, depth of field.
* Ellipsoids, boxes, cylinders and cones.
* Blue noise sampling.
* Grid acceleration structure.
* Edit entities.
* Save/load scenes.
* Save screenshots.

Runs on Windows 32bit and 64bit.
It uses Opengl 3.3, Freetype to render fonts and stb_image/stb_image_write to read/write images.
Lines of code: 13.8K.

#### Controls

W,A,S,D         - Move camera.  
Q,E             - Move camera vertically.  
Shift + W,A,S,D - Move faster.  
RightMB         - Move camera view.  
F3              - Toggle first person mode.  
  
R               - Toggle between local/global translation/rotation.  
1,2,3,MWheel    - Select translate/rotate/scale editing modes.  
Ctrl + A        - Select all.  
Ctrl + X        - Cut.  
Ctrl + C        - Copy.  
Ctrl + V        - Paste.  
MiddleMB        - Drag selection.  
Ctrl + LeftMB   - Add/subtract selection.  
Ctrl + Y        - Undo.  
Ctrl + Z        - Redo.  
Shift           - Enable snapping.  
Ctrl            - Scale equally when resizing.  
  
Space           - Render from current camera view.  
Escape          - Abort rendering.  
F               - Toggle fit to screen.  
Tab             - Hide panels.  
F11             - Fullscreen mode.  
