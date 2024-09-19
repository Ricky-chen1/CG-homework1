## 重新编译项目

根目录下运行

```bash
# cmake构建编译
cmake -S . -B build
cmake --build build

# 生成三角形图像
./build/Rasterizer
```

## 主要实现函数

### 判断点是否位于三角形内部

``static bool insideTriangle(float x, float y, const Vector3f* _v)``

计算点与三角形顶点向量与边的叉乘，三次同号则点位于三角形内部

### 光栅化三角形并利用超采样实现抗锯齿

``void rst::rasterizer::rasterize_triangle(const Triangle& t)``

1. 获取三角形的bounding box（即边界）
2. 遍历box中每个像素点，采样4个点（2 * 2采样实现抗锯齿）
3. 若采样点在三角形内部，则运用z-buffer，设置更靠近相机点的深度缓冲和颜色。

## 最终结果

### 超采样前

![images](https://github.com/Ricky-chen1/CG-homework1/blob/main/images/before.png)

### 超采样后

![images](https://github.com/Ricky-chen1/CG-homework1/blob/main/images/after.png)