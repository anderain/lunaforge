import fs from 'fs';
import path from 'path';
import { execSync } from 'child_process';

/**
 * 工具配置接口包
 * @property {string} spriteSlicer 精灵分割器路径
 */
interface ToolConfig {
  spriteSlicer: string;
}

/**
 * 项目配置接口
 * @property {string} projectName - 项目名称
 * @property {string} alias - 项目别名(1-8个大写字母/数字)
 * @property {string} assetsFolder - 资源文件夹路径(仅字母)
 * @property {string} cacheFolder - 缓存文件夹路径(仅字母)
 * @property {ToolConfig} tools - 工具配置
 */
interface ProjectConfig {
  projectName: string;
  alias: string;
  assetsFolder: string;
  cacheFolder: string;
  tools: ToolConfig;
}

/**
 * 验证项目配置是否有效
 * @param {ProjectConfig} config - 要验证的项目配置
 * @throws {Error} 当配置无效时抛出错误
 */
function validateConfig(config: ProjectConfig): void {
  if (!config.projectName || typeof config.projectName !== 'string') {
    throw new Error('Invalid projectName: must be a non-empty string');
  }

  if (!/^[A-Z0-9]{1,8}$/.test(config.alias)) {
    throw new Error('Invalid alias: must be 1-8 uppercase letters/numbers');
  }

  if (!/^[a-zA-Z]+$/.test(config.assetsFolder)) {
    throw new Error('Invalid assetsFolder: must contain only letters');
  }

  if (!/^[a-zA-Z]+$/.test(config.cacheFolder)) {
    throw new Error('Invalid cacheFolder: must contain only letters');
  }

  if (!config.tools?.spriteSlicer) {
    throw new Error('Missing required tool: spriteSlicer');
  }
}

/**
 * 检查资产文件名是否有效
 * @param {string} file - 要检查的文件名
 * @param {string} ext - 文件扩展名
 * @return {boolean} 文件名是否有效
 */
function isValidAssetFileName(file: string, ext: string) {
  const regex = new RegExp(`^[a-zA-Z0-9_]{1,32}\.${ext}$`)
  return regex.test(file)
}

/**
 * 获取目录中所有BMP文件
 * @param {string} dir - 要搜索的目录路径
 * @return {string[]} BMP文件名数组
 */
function getBmpFiles(dir: string): string[] {
  const files = fs.readdirSync(dir);
  return files.filter(file =>
    file.toLowerCase().endsWith('.bmp')
  );
}

/**
 * 处理BMP文件，使用精灵分割器转换格式
 * @param {string} bmpPath - BMP文件路径
 * @param {string|null} metaPath - 元数据文件路径(可选)
 * @param {string} binPath - 输出二进制文件路径
 * @param {string} spriteSlicerPath - 精灵分割器工具路径
 * @throws {Error} 当处理失败时抛出错误
 */
function processBmpFile(
  bmpPath: string,
  metaPath: string | null,
  binPath: string,
  spriteSlicerPath: string
): void {
  const args = [
    `"${bmpPath}"`,
    '-f binary',
    ...(metaPath ? [`-m "${metaPath}"`] : []),
    `-o "${binPath}"`
  ].join(' ');

  try {
    execSync(`${spriteSlicerPath} ${args}`, { stdio: 'inherit' });
  } catch (error) {
    throw new Error(`Failed to process ${bmpPath}: ${error}`);
  }
}

/**
 * 主函数，执行BMP文件处理流程
 */
function main() {
  try {
    // 从命令行获取配置文件路径
    const configPath = process.argv[2];
    if (!configPath) {
      throw new Error('Please provide path to config file');
    }

    // 加载并验证配置
    const projectPath = path.dirname(path.resolve(configPath));
    const config: ProjectConfig = JSON.parse(fs.readFileSync(configPath, 'utf-8'));
    validateConfig(config);

    // 准备路径
    const assetsPath = path.join(projectPath, config.assetsFolder);
    const cachePath = path.join(projectPath, config.cacheFolder);

    // 确保缓存目录存在
    if (!fs.existsSync(cachePath)) {
      fs.mkdirSync(cachePath, { recursive: true });
    }

    // 处理所有BMP文件
    const bmpFiles = getBmpFiles(assetsPath);
    for (const bmpFile of bmpFiles) {
      if (!isValidAssetFileName(bmpFile, 'bmp')) {
        throw new Error(`Invalid bmp file name ${bmpFile}`);
      }
      const bmpPath = path.join(assetsPath, bmpFile);
      const metaPath = path.join(assetsPath, `${path.parse(bmpFile).name}.meta.txt`);
      const binPath = path.join(cachePath, `${path.parse(bmpFile).name}`);

      const hasMeta = fs.existsSync(metaPath);
      const bmpTime = fs.statSync(bmpPath).mtimeMs;
      const metaTime = hasMeta ? fs.statSync(metaPath).mtimeMs : 0;
      const binTime = fs.existsSync(binPath) ? fs.statSync(binPath).mtimeMs : 0;

      if (bmpTime > binTime || (hasMeta && metaTime > binTime)) {
        console.log(`Processing ${bmpFile}...`);
        processBmpFile(
          bmpPath,
          hasMeta ? metaPath : null,
          binPath,
          config.tools.spriteSlicer
        );
      } else {
        console.log(`Skipping ${bmpFile} (up to date)`);
      }
    }

    console.log('Processing completed successfully!');
  } catch (error) {
    console.error('Error:', error instanceof Error ? error.message : error);
    process.exit(1);
  }
}

main();