// VGA Mode 13h 调色板，忽略最后8个纯黑色
const VgaMode13hColorPalette = [
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
    0x000000, 0x101010, 0x202020, 0x353535, 0x454545, 0x555555, 0x656565, 0x757575,
    0x8A8A8A, 0x9A9A9A, 0xAAAAAA, 0xBABABA, 0xCACACA, 0xDFDFDF, 0xEFEFEF, 0xFFFFFF,
    0x0000FF, 0x4100FF, 0x8200FF, 0xBE00FF, 0xFF00FF, 0xFF00BE, 0xFF0082, 0xFF0041,
    0xFF0000, 0xFF4100, 0xFF8200, 0xFFBE00, 0xFFFF00, 0xBEFF00, 0x82FF00, 0x41FF00,
    0x00FF00, 0x00FF41, 0x00FF82, 0x00FFBE, 0x00FFFF, 0x00BEFF, 0x0082FF, 0x0041FF,
    0x8282FF, 0x9E82FF, 0xBE82FF, 0xDF82FF, 0xFF82FF, 0xFF82DF, 0xFF82BE, 0xFF829E,
    0xFF8282, 0xFF9E82, 0xFFBE82, 0xFFDF82, 0xFFFF82, 0xDFFF82, 0xBEFF82, 0x9EFF82,
    0x82FF82, 0x82FF9E, 0x82FFBE, 0x82FFDF, 0x82FFFF, 0x82DFFF, 0x82BEFF, 0x829EFF,
    0xBABAFF, 0xCABAFF, 0xDFBAFF, 0xEFBAFF, 0xFFBAFF, 0xFFBAEF, 0xFFBADF, 0xFFBACA,
    0xFFBABA, 0xFFCABA, 0xFFDFBA, 0xFFEFBA, 0xFFFFBA, 0xEFFFBA, 0xDFFFBA, 0xCAFFBA,
    0xBAFFBA, 0xBAFFCA, 0xBAFFDF, 0xBAFFEF, 0xBAFFFF, 0xBAEFFF, 0xBADFFF, 0xBACAFF,
    0x000071, 0x1C0071, 0x390071, 0x550071, 0x710071, 0x710055, 0x710039, 0x71001C,
    0x710000, 0x711C00, 0x713900, 0x715500, 0x717100, 0x557100, 0x397100, 0x1C7100,
    0x007100, 0x00711C, 0x007139, 0x007155, 0x007171, 0x005571, 0x003971, 0x001C71,
    0x393971, 0x453971, 0x553971, 0x613971, 0x713971, 0x713961, 0x713955, 0x713945,
    0x713939, 0x714539, 0x715539, 0x716139, 0x717139, 0x617139, 0x557139, 0x457139,
    0x397139, 0x397145, 0x397155, 0x397161, 0x397171, 0x396171, 0x395571, 0x394571,
    0x515171, 0x595171, 0x615171, 0x695171, 0x715171, 0x715169, 0x715161, 0x715159,
    0x715151, 0x715951, 0x716151, 0x716951, 0x717151, 0x697151, 0x617151, 0x597151,
    0x517151, 0x517159, 0x517161, 0x517169, 0x517171, 0x516971, 0x516171, 0x515971,
    0x000041, 0x100041, 0x200041, 0x310041, 0x410041, 0x410031, 0x410020, 0x410010,
    0x410000, 0x411000, 0x412000, 0x413100, 0x414100, 0x314100, 0x204100, 0x104100,
    0x004100, 0x004110, 0x004120, 0x004131, 0x004141, 0x003141, 0x002041, 0x001041,
    0x202041, 0x282041, 0x312041, 0x392041, 0x412041, 0x412039, 0x412031, 0x412028,
    0x412020, 0x412820, 0x413120, 0x413920, 0x414120, 0x394120, 0x314120, 0x284120,
    0x204120, 0x204128, 0x204131, 0x204139, 0x204141, 0x203941, 0x203141, 0x202841,
    0x2D2D41, 0x312D41, 0x352D41, 0x3D2D41, 0x412D41, 0x412D3D, 0x412D35, 0x412D31,
    0x412D2D, 0x41312D, 0x41352D, 0x413D2D, 0x41412D, 0x3D412D, 0x35412D, 0x31412D,
    0x2D412D, 0x2D4131, 0x2D4135, 0x2D413D, 0x2D4141, 0x2D3D41, 0x2D3541, 0x2D3141
];

/**
 * 将十六进制颜色值转换为 RGB 结构。
 * @param   {number} hex - 十六进制颜色值（例如 0xRRGGBB）。
 * @returns {{r: number, g: number, b: number}} 
 *          返回包含红、绿、蓝分量的对象。
 */
function hexToRgb(hex) {
    return {
        r: (hex >> 16) & 0xFF,
        g: (hex >> 8) & 0xFF,
        b: hex & 0xFF
    };
}

/**
 * 将 RGB 颜色转换为 HSL 颜色。
 * @param   {{r: number, g: number, b: number}} rgb - RGB 颜色对象，
 *          其中 r、g、b 为 0-255 的整数。
 * @returns {{h: number, s: number, l: number}} 
 *          返回 HSL 颜色对象，h 为色相（0-360），
 *          s 和 l 为饱和度与亮度（0-1）。
 * @note    当 RGB 三个通道值相等时，表示灰色，色相和饱和度将为 0。
 */
function rgbToHsl({ r, g, b }) {
    r /= 255;
    g /= 255;
    b /= 255;
    const max = Math.max(r, g, b), min = Math.min(r, g, b);
    let h, s, l = (max + min) / 2;

    if (max === min) {
        h = s = 0; // 灰色
    } else {
        const d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        switch (max) {
            case r: h = ((g - b) / d + (g < b ? 6 : 0)); break;
            case g: h = ((b - r) / d + 2); break;
            case b: h = ((r - g) / d + 4); break;
        }
        h /= 6;
    }

    return { h: h * 360, s, l };
}

/**
 * 创建一个扇形的 SVG path 字符串
 * @param   {number} cx 圆心X
 * @param   {number} cy 圆心Y
 * @param   {number} r 半径
 * @param   {number} startAngle 起始角度（单位：度）
 * @param   {number} endAngle 结束角度（单位：度）
 * @param   {string} fill 填充样式
 * @returns {string} SVG <path> 元素字符串
 */
function createSector(cx, cy, r, startAngle, endAngle, fill) {
  // 将角度转换为弧度
  const rad = Math.PI / 180;
  const startRad = startAngle * rad;
  const endRad = endAngle * rad;

  // 起点
  const x1 = cx + r * Math.cos(startRad);
  const y1 = cy + r * Math.sin(startRad);

  // 终点
  const x2 = cx + r * Math.cos(endRad);
  const y2 = cy + r * Math.sin(endRad);

  // 是否大弧（大于180°）
  const largeArcFlag = (endAngle - startAngle) % 360 > 180 ? 1 : 0;

  // SVG Path: 从圆心 -> 起点 -> 圆弧 -> 回到圆心
  const d = [
    `M ${cx} ${cy}`, // 移动到圆心
    `L ${x1} ${y1}`, // 画直线到起点
    `A ${r} ${r} 0 ${largeArcFlag} 1 ${x2} ${y2}`, // 画圆弧
    'Z' // 闭合路径
  ].join(' ');

  return `<path d="${d}" fill="${fill}" />`;
}

// 解析颜色
const parsedColors = VgaMode13hColorPalette.map(hex => {
    const rgb = hexToRgb(hex);
    const hsl = rgbToHsl(rgb);
    return { hex, ...rgb, ...hsl };
});

// 分组：灰阶 vs 彩色
const grayColors = parsedColors.filter(c => c.s < 0.05); // 低饱和度
const colorColors = parsedColors.filter(c => c.s >= 0.05);

// 彩色按 hue 排序
colorColors.sort((a, b) => a.h - b.h);

// SVG 配置
const size = 800;
const cx = size / 2;
const cy = size / 2;
const shapes = [];

// 计算需要多少圈圆环
const ringRadiusMax = 20;
const availableRingRadius = {}
for (const c of colorColors) {
    const radius = Math.floor((1 - c.l) * ringRadiusMax);
    availableRingRadius[radius] = true;
}

// 创建半径 -> 圆环下标的映射
const ringRadiusIndexMap = {};
Object.keys(availableRingRadius).sort((a, b) => a > b).forEach((radius, index) => {
    ringRadiusIndexMap[radius] = index;
})

// 创建圆环，把颜色按照亮度放到不同的环中
const rings = new Array(Object.keys(ringRadiusIndexMap).length).fill(0).map(() => [])
for (const c of colorColors) {
    const radius = Math.floor((1 - c.l) * ringRadiusMax);
    const ringIndex = ringRadiusIndexMap[radius];
    rings[ringIndex].push(c)
}

// 对圆环的每个环内容进行排序，根据色相排序
for (let index in rings) {
    rings[index] = rings[index].sort((colorA, colorB) => colorA.h - colorB.h)
}

// 把灰度颜色放置在最外圈
rings.push(grayColors.sort((grayA, grayB) => grayB.l - grayA.l))

// 绘制圆环
const sectorMaxRadius = 350;
const centerRadius = 15;
const radiusOffset = 40;
// 从外向里绘制，越靠近中心越绘制，置于上层
for (let i = rings.length - 1; i >= 0; --i) {
    const avgAngleDeg = 360 / rings[i].length;
    const radius = (sectorMaxRadius + centerRadius) / rings.length * (i + 1) + (radiusOffset * (1 / (i + 2)));
    for (let j = 0; j < rings[i].length; ++j) {
        const color = rings[i][j];
        const fill = `rgb(${color.r}, ${color.g}, ${color.b})`;
        // 红色放置于最顶
        const startAngle = avgAngleDeg * (j - 0.5) - 90;
        // 稍微多填充0.5deg来避免缝隙
        const endAngle = startAngle + avgAngleDeg + 0.5;
        shapes.push(createSector(cx, cy, radius, startAngle, endAngle, fill));
    }
}
// 添加中心的白色圆形
shapes.push(`<circle cx="${cx}" cy="${cy}" r="${centerRadius}" fill="#fff" />`);

// 绘制方形的进行测试
// for (let i in rings) {
//     for (let j in rings[i]) {
//         const c = rings[i][j];
//         const w = 10;
//         const h = 10;
//         const x = i * w;
//         const y = j * h;
//         const fill = `rgb(${c.r}, ${c.g}, ${c.b})`
//         shapes.push(`<rect x="${x}" y="${y}" width="${w}" height="${h}" fill="${fill}" />`)
//     }
// }

// SVG 拼接
const svgContent = `
<svg xmlns="http://www.w3.org/2000/svg" width="${size}" height="${size}">
  <rect width="100%" height="100%" fill="white"/>
  ${shapes.join('\n')}
</svg>
`;
console.log(svgContent.trim());