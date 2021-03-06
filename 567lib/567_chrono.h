#pragma once
#include <time.h>
#include <chrono>

namespace _567 {
	/************************************************************************************/
	// time_point <--> .net DateTime.Now.ToUniversalTime().Ticks converts

	// 经历时间精度: 秒后 7 个 0( 这是 windows 下最高精度. android/ios 会低1个0的精度 )
	typedef std::chrono::duration<long long, std::ratio<1LL, 10000000LL>> duration_10m;

	// 天
	typedef std::chrono::duration<long long, std::ratio<24*60*60>> duration_day;

	// 时间点 转 epoch (精度为秒后 7 个 0)
	inline int64_t TimePointToEpoch10m(std::chrono::system_clock::time_point const& val) noexcept {
		return std::chrono::duration_cast<duration_10m>(val.time_since_epoch()).count();
	}

	//  epoch (精度为秒后 7 个 0) 转 时间点
	inline std::chrono::system_clock::time_point Epoch10mToTimePoint(int64_t const& val) noexcept {
		return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration_10m(val)));
	}


	// 得到当前时间点
	inline std::chrono::system_clock::time_point Now() noexcept {
		return std::chrono::system_clock::now();
	}
	inline std::chrono::system_clock::time_point NowTimePoint() noexcept {
		return std::chrono::system_clock::now();
	}
	
	inline std::chrono::steady_clock::time_point NowSteadyTimePoint() noexcept {
		return std::chrono::steady_clock::now();
	}

	// 得到今日凌晨0点0分0秒的时间 精度(秒)
	inline uint64_t Today() noexcept {
        return std::chrono::duration_cast<duration_day>(Now().time_since_epoch()).count() * 86400;
	}

	// 得到明天凌晨0点0分0秒的时间 精度(秒)
	inline uint64_t Tomorrow() noexcept {
        return (std::chrono::duration_cast<duration_day>(Now().time_since_epoch()).count() + 1) * 86400;
	}

	// 得到当前时间点的 epoch (精度为秒后 7 个 0)
	inline int64_t NowEpoch10m() noexcept {
		return TimePointToEpoch10m(NowTimePoint());
	}
	
	// 得到当前时间点的 epoch 微秒
	inline int64_t NowEpochMicroseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	// 得到当前时间点的 微秒 精度( 秒后 6 位 )
	inline int64_t NowMicroseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

    // 得到当前 system 时间点的 epoch (精度为秒)
    inline int64_t NowSystemEpochS() noexcept {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 得到当前 system 时间点的 epoch (精度为 毫秒)
    inline int64_t NowSystemEpochMS() noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 得到当前时间点的 epoch 毫秒
    inline int64_t NowEpochMilliseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // 得到当前时间点的 epoch 秒
    inline double NowEpochSeconds() noexcept {
        return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000000.0;
    }
    // 得到当前时间点的 epoch 秒 更新并返回和 last 的时间差. last 可用 NowEpochSeconds 初始化
    inline double NowEpochSeconds(double& last) noexcept {
        auto now = NowEpochSeconds();
        auto rtv = now - last;
        last = now;
        return rtv;
    }

	// epoch (精度为秒后 7 个 0) 转为 .Net DateTime Utc Ticks
	inline int64_t Epoch10mToUtcDateTimeTicks(int64_t const& val) noexcept {
		return val + 621355968000000000LL;
	}

	// .Net DateTime Utc Ticks 转为 epoch (精度为秒后 7 个 0)
	inline int64_t UtcDateTimeTicksToEpoch10m(int64_t const& val) noexcept {
		return val - 621355968000000000LL;
	}


	// 时间点 转 epoch (精度为秒)
	inline int32_t TimePointToEpoch(std::chrono::system_clock::time_point const& val) noexcept {
		return (int32_t)(val.time_since_epoch().count() / 10000000);
	}

	//  epoch (精度为秒) 转 时间点
	inline std::chrono::system_clock::time_point EpochToTimePoint(int32_t const& val) noexcept {
		return std::chrono::system_clock::time_point(std::chrono::system_clock::time_point::duration((int64_t)val * 10000000));
	}

    // 得到当前时间点的 epoch ticks(精度为秒后 7 个 0)
    inline int64_t NowSteadyEpoch10m() noexcept {
        return TimePointToEpoch10m(NowTimePoint());
    }

    // 得到当前时间点的 epoch 微妙
    inline int64_t NowSteadyEpochMicroseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // 得到当前时间点的 epoch 毫秒
    inline int64_t NowSteadyEpochMilliseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // 得到当前时间点的 epoch 秒
    inline double NowSteadyEpochSeconds() noexcept {
        return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1000000.0;
    }
    // 得到当前时间点的 epoch 秒 更新并返回和 last 的时间差. last 可用 NowEpochSeconds 初始化
    inline double NowSteadyEpochSeconds(double& last) noexcept {
        auto now = NowSteadyEpochSeconds();
        auto rtv = now - last;
        last = now;
        return rtv;
    }

    // 得到今天是这个星期的星期几
    inline short int NowDayIndexInWeek() noexcept {
        time_t tmNow = time(NULL);
        tm* ptmNow = localtime(&tmNow);
        return ptmNow->tm_wday;
	}

	// 本周日的0点0分0秒, 精度秒
	inline int64_t Sunday() noexcept {
        auto weekDay = NowDayIndexInWeek();
        return Today() - 86400 * weekDay;
	}

    // 本周一的0点0分0秒, 精度秒
	inline int64_t Monday() noexcept {
	    auto weekDay = NowDayIndexInWeek();
        return Today() - 86400 * (weekDay - 1);
	}

    // 本周二的0点0分0秒, 精度秒
	inline int64_t Tuesday() noexcept {
        auto weekDay = NowDayIndexInWeek();
        return Today() - 86400 * (weekDay - 2);
	}

    // 本周三的0点0分0秒, 精度秒
	inline int64_t Wednesday() noexcept {
        auto weekDay = NowDayIndexInWeek();
        return Today() - 86400 * (weekDay - 3);
	}

    // 本周四的0点0分0秒, 精度秒
	inline int64_t Thursday() noexcept {
        auto weekDay = NowDayIndexInWeek();
        return Today() - 86400 * (weekDay - 4);
	}

    // 本周五的0点0分0秒, 精度秒
	inline int64_t Friday() noexcept {
        auto weekDay = NowDayIndexInWeek();
        return Today() - 86400 * (weekDay - 5);
	}

    // 本周六的0点0分0秒, 精度秒
	inline int64_t Saturday() noexcept {
        auto weekDay = NowDayIndexInWeek();
        return Today() - 86400 * (weekDay - 6);
	}


//	// 当前时间转为字符串并填充
//	inline void NowToString(std::string& s) noexcept {
//		auto&& t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
//		std::tm tm;
//#ifdef _WIN32
//		localtime_s(&tm, &t);
//#else
//		localtime_r(&t, &tm);
//#endif
//		std::stringstream ss;
//		ss << std::put_time(&tm, "%Y-%m-%d %X");
//		s += ss.str();
//	}
//
//	// 当前时间转为字符串并返回
//	inline std::string NowToString() noexcept {
//		std::string s;
//		NowToString(s);
//		return s;
//	}
}
