#pragma once
#include <QVariantMap>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

struct PreviewExposureLut {
    static constexpr int size=1024;
    std::array<unsigned char,size*2*4> pixels{};
    std::array<float,9> matrix{}; // OpenGL column-major.
    bool valid=false;
    explicit PreviewExposureLut(const QVariantMap &reference) {
        using Point=std::array<double,2>;
        const auto read=[](const QVariant &value) {
            std::vector<Point> points;
            const auto list=value.toList();
            if (list.size()<2 || list.size()>1000) return points;
            for (const auto &v:list) {
                auto p=v.toList();
                if (p.size()!=2) return std::vector<Point>{};
                bool xOk=false,yOk=false;
                const double x=p[0].toDouble(&xOk),y=p[1].toDouble(&yOk);
                if (!xOk || !yOk || !std::isfinite(x) || !std::isfinite(y) ||
                    (!points.empty() && x<=points.back()[0])) return std::vector<Point>{};
                points.push_back({x,y});
            }
            return points;
        };
        const auto decode=read(reference.value("decode_curve")),display=read(reference.value("display_curve"));
        const auto m=reference.value("to_rec709").toList();
        if (decode.empty() || display.empty() || m.size()!=9) return;
        for (int r=0;r<3;++r) for (int c=0;c<3;++c) {
            bool ok=false; const double value=m[r*3+c].toDouble(&ok);
            if (!ok || !std::isfinite(value)) return;
            matrix[c*3+r]=float(value);
        }
        const auto sample=[](const std::vector<Point> &points,double x) {
            auto upper=std::upper_bound(points.begin(),points.end(),x,[](double x,const Point &p){return x<p[0];});
            if (upper==points.begin()) return points.front()[1];
            if (upper==points.end()) return points.back()[1];
            const auto &b=*upper,&a=*(upper-1);
            return a[1]+(x-a[0])/(b[0]-a[0])*(b[1]-a[1]);
        };
        for (int row=0;row<2;++row) for (int x=0;x<size;++x) {
            const unsigned value=unsigned(std::lround(std::clamp(sample(row ? display : decode,x/double(size-1)),0.0,1.0)*65535));
            const int index=(row*size+x)*4;
            pixels[index]=value>>8; pixels[index+1]=value&255; pixels[index+3]=255;
        }
        valid=true;
    }
};
