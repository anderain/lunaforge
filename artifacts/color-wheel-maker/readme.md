# Color Wheel Maker  
此程序会把 VGA Mode 13H 的 256 色调色板转为一个色轮的 SVG 图片。  
This program converts the 256-color palette of VGA Mode 13H into an SVG format of a color wheel.  

此程序只是提供给美术人员进行参考，并不是运行时或者编辑器使用的工具。  
This program is intended as a reference for artists and is not a runtime or editor tool.  

### 输出 / Output
- 输出 SVG 格式的文本到 stdout。  
  Outputs SVG-format text to stdout.  
- 灰阶颜色在最外层。其他颜色按照亮度从外到内排列在不同的圆环上。  
  Grayscale colors are placed on the outermost layer. Other colors are arranged from outer to inner rings according to brightness.  
- 圆环上的颜色按照色相进行排序。  
  Colors on each ring are sorted by hue.  

### 使用方法 / Usage  
```bash
node wheel.js > wheel.svg
magick -density 300 wheel.svg wheel.png
```