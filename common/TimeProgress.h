#ifndef TIME_PROGRESS_H
#define TIME_PROGRESS_H

#include <algorithm>

inline double computeProgress(long long startTime, long long endTime, long long currentTime)
{
    if (endTime <= startTime)
    {
        return 1.0;
    }

    double raw = static_cast<double>(currentTime - startTime) /
                 static_cast<double>(endTime - startTime);

    return std::min(1.0, std::max(0.0, raw));
}

#endif
