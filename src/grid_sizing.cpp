#include "css_grid.h"
#include <algorithm>
#include <cmath>

namespace litehtml
{
    bool distribute_grid_fractions(pixel_t available, const std::vector<float>& fractions, std::vector<pixel_t>& sizes)
    {
        if(fractions.size() != sizes.size() || !std::isfinite(available.value()))
        {
            return false;
        }
        std::vector<bool> frozen(fractions.size(), false);
        float             free_space = available.value();
        float             factor_sum = 0;
        for(size_t index = 0; index < fractions.size(); ++index)
        {
            if(!std::isfinite(fractions[index]) || fractions[index] < 0 || !std::isfinite(sizes[index].value()) ||
               sizes[index] < 0_px)
            {
                return false;
            }
            if(fractions[index] > 0)
            {
                factor_sum += fractions[index];
            } else
            {
                frozen[index]  = true;
                free_space    -= sizes[index].value();
            }
        }
        if(!std::isfinite(factor_sum))
        {
            return false;
        }
        for(size_t iteration = 0; iteration < fractions.size(); ++iteration)
        {
            const float fraction_size = std::max(0.0f, free_space) / std::max(1.0f, factor_sum);
            bool        changed       = false;
            for(size_t index = 0; index < fractions.size(); ++index)
            {
                if(!frozen[index] && fraction_size * fractions[index] < sizes[index].value())
                {
                    frozen[index]  = true;
                    free_space    -= sizes[index].value();
                    factor_sum    -= fractions[index];
                    changed        = true;
                }
            }
            if(!changed)
            {
                break;
            }
        }
        const float fraction_size = std::max(0.0f, free_space) / std::max(1.0f, factor_sum);
        for(size_t index = 0; index < fractions.size(); ++index)
        {
            if(!frozen[index])
            {
                sizes[index] = pixel_t(fraction_size * fractions[index]);
            }
        }
        return true;
    }
} // namespace litehtml
