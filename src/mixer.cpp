#include "hush/mixer.hpp"
#include <algorithm>
#include <cmath>
namespace hush {
StereoBuffer mix_tracks(const Project& p,const std::vector<std::vector<float>>& inputs) {
    std::size_t frames=0;
    for(const auto& v:inputs) frames=std::max(frames,v.size());
    StereoBuffer out{std::vector<float>(frames,0.0F),std::vector<float>(frames,0.0F)};
    const bool any_solo=std::any_of(p.tracks.begin(),p.tracks.end(),[](const Track& t){return t.solo;});
    constexpr float pi=3.14159265358979323846F;
    for(std::size_t n=0;n<p.tracks.size() && n<inputs.size();++n) {
        const auto& t=p.tracks[n];
        if(t.mute || (any_solo && !t.solo)) continue;
        const float pan=std::clamp(t.pan,-1.0F,1.0F);
        float l=0.0F,r=0.0F;
        if(p.pan_law==PanLaw::LegacyMinus4_5dB) {
            l=pan>0.0F?1.0F-pan:1.0F;
            r=pan<0.0F?1.0F+pan:1.0F;
            constexpr float center=0.5946035575F;
            if(std::abs(pan)<1.0e-6F) l=r=center;
        } else {
            l=std::cos((pan+1.0F)*pi*0.25F);
            r=std::sin((pan+1.0F)*pi*0.25F);
        }
        for(std::size_t i=0;i<inputs[n].size();++i) {
            const float s=inputs[n][i]*t.gain;
            out.left[i]+=s*l; out.right[i]+=s*r;
        }
    }
    return out;
}
}
