// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>
#include <tuple>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(float x, float y, const Vector3f* _v)
{   
    // TODO: Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
     // 三角形的三个顶点
    Vector3f p0 = _v[0];
    Vector3f p1 = _v[1];
    Vector3f p2 = _v[2];

    // 计算边向量
    Vector3f v0 = p1 - p0;
    Vector3f v1 = p2 - p1;
    Vector3f v2 = p0 - p2;

    // 计算点与三角形顶点的向量
    Vector3f point(x, y, 0);
    Vector3f c0 = point - p0;
    Vector3f c1 = point - p1;
    Vector3f c2 = point - p2;

    // 计算叉乘
    float cross0 = v0.x() * c0.y() - v0.y() * c0.x();
    float cross1 = v1.x() * c1.y() - v1.y() * c1.x();
    float cross2 = v2.x() * c2.y() - v2.y() * c2.x();

    // 检查叉乘的符号是否相同
    if ((cross0 > 0 && cross1 > 0 && cross2 > 0) || (cross0 < 0 && cross1 < 0 && cross2 < 0)) {
        return true;
    }

    return false;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    // 获取三角形的顶点
    auto v = t.toVector4();
    
    // 得到bounding box边界
    float minX = std::min({v[0].x(), v[1].x(), v[2].x()});
    float maxX = std::max({v[0].x(), v[1].x(), v[2].x()});
    float minY = std::min({v[0].y(), v[1].y(), v[2].y()});
    float maxY = std::max({v[0].y(), v[1].y(), v[2].y()});

    for (int x = std::floor(minX); x <= std::ceil(maxX); ++x) {
        for (int y = std::floor(minY); y <= std::ceil(maxY); ++y) {
            // 每个像素的采样点
            std::vector<Eigen::Vector2f> sample_list = {
                {x + 0.25f, y + 0.25f}, {x + 0.75f, y + 0.25f},
                {x + 0.25f, y + 0.75f}, {x + 0.75f, y + 0.75f}
            };

            // 遍历该像素的四个采样点
            for (int i = 0; i < 4; i++) {
                float sample_x = sample_list[i].x();
                float sample_y = sample_list[i].y();
                // 如果采样点在三角形内部
                if (insideTriangle(sample_x, sample_y, t.v)) {
                    // 插值法计算深度值
                    float alpha, beta, gamma;
                    std::tie(alpha, beta, gamma) = computeBarycentric2D(sample_x, sample_y, t.v);

                    float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    z_interpolated *= w_reciprocal;

                    int index = get_index(x, y);
                    // 比较当前采样点的深度缓冲值并更新（z-buffer）
                    if (z_interpolated < sample_depth_buf[index][i]) {
                        sample_depth_buf[index][i] = z_interpolated;
                        sample_color_buf[index][i] = t.getColor();
                    }
                }
            }
        }
    }

    // 综合采样点的颜色
    for (int x = std::floor(minX); x <= std::ceil(maxX); ++x) {
        for (int y = std::floor(minY); y <= std::ceil(maxY); ++y) {
            int index = get_index(x, y);
            Eigen::Vector3f final_color(0, 0, 0);
            // 平均采样点颜色
            for (int i = 0; i < 4; i++) {
                final_color += sample_color_buf[index][i];
            }
            final_color /= 4.0; // 4个采样点
            // 使用set_pixel函数设置像素颜色
            set_pixel(Eigen::Vector3f(x, y, 0), final_color);
        }
    }
}


void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    // 单点采样时
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);

    // 初始化每个像素的4个采样点深度缓冲和颜色缓冲
    sample_depth_buf.resize(w * h, std::vector<float>(4, std::numeric_limits<float>::infinity()));
    sample_color_buf.resize(w * h, std::vector<Eigen::Vector3f>(4, Eigen::Vector3f(0, 0, 0)));
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on