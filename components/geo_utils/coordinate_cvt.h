/**
 * 百度坐标（BD09）、国测局坐标（火星坐标，GCJ02）、和WGS84坐标系之间的转换的工具
 * referency from a java implement https://github.com/wandergis/coordtransform
 */
#ifndef _COMPONENT_GEOUTILS_COORDCVT_H_H
#define _COMPONENT_GEOUTILS_COORDCVT_H_H

#include <math.h>

namespace geo {

static const double x_pi = 3.14159265358979324 * 3000.0 / 180.0;
// π
static const double pi = 3.1415926535897932384626;
// 长半轴
static const double a = 6378245.0;
// 扁率
static const double ee = 0.00669342162296594323;

typedef struct LngLatPair {
  double lng;
  double lat;
} LngLatPair;


class CoordinateTransformUtil {
  public:

  /**
   * 摩托卡坐标系to平面坐标系
   */
  static LngLatPair mercator2ll(double bdmc_lon, double bdmc_lat) {
    double x = bdmc_lon/20037726.3545186*180;
    double y = bdmc_lat/19918000*180;
    y= 180 / M_PI * ( 2 * ::atan(::exp(y * M_PI / 180)) - M_PI / 2);
    return {x, y};
  }
	/**
	 * 百度坐标系(BD-09)转WGS坐标
	 *
	 * @param lng 百度坐标纬度
	 * @param lat 百度坐标经度
	 * @return WGS84坐标数组
	 */
	static LngLatPair bd09towgs84(double lng, double lat) {
		LngLatPair gcj = bd09togcj02(lng, lat);
		LngLatPair wgs84 = gcj02towgs84(gcj.lng, gcj.lat);
		return wgs84;
	}

	/**
	 * WGS坐标转百度坐标系(BD-09)
	 *
	 * @param lng WGS84坐标系的经度
	 * @param lat WGS84坐标系的纬度
	 * @return 百度坐标数组
	 */
	static LngLatPair wgs84tobd09(double lng, double lat) {
		LngLatPair gcj = wgs84togcj02(lng, lat);
		LngLatPair bd09 = gcj02tobd09(gcj.lng, gcj.lat);
		return bd09;
	}

	/**
	 * 火星坐标系(GCJ-02)转百度坐标系(BD-09)
	 *
	 * 谷歌、高德——>百度
	 * @param lng 火星坐标经度
	 * @param lat 火星坐标纬度
	 * @return 百度坐标数组
	 */
	static LngLatPair gcj02tobd09(double lng, double lat) {
		double z = ::sqrt(lng * lng + lat * lat) + 0.00002 * ::sin(lat * x_pi);
		double theta = ::atan2(lat, lng) + 0.000003 * ::cos(lng * x_pi);
		double bd_lng = z * ::cos(theta) + 0.0065;
		double bd_lat = z * ::sin(theta) + 0.006;
		return { bd_lng, bd_lat };
	}

	/**
	 * 百度坐标系(BD-09)转火星坐标系(GCJ-02)
	 *
	 * 百度——>谷歌、高德
	 * @param bd_lon 百度坐标纬度
	 * @param bd_lat 百度坐标经度
	 * @return 火星坐标数组
	 */
	static LngLatPair bd09togcj02(double bd_lon, double bd_lat) {
		double x = bd_lon - 0.0065;
		double y = bd_lat - 0.006;
		double z = ::sqrt(x * x + y * y) - 0.00002 * ::sin(y * x_pi);
		double theta = ::atan2(y, x) - 0.000003 * ::cos(x * x_pi);
		double gg_lng = z * ::cos(theta);
		double gg_lat = z * ::sin(theta);
		return { gg_lng, gg_lat };
	}

	/**
	 * WGS84转GCJ02(火星坐标系)
	 *
	 * @param lng WGS84坐标系的经度
	 * @param lat WGS84坐标系的纬度
	 * @return 火星坐标数组
	 */
	static LngLatPair wgs84togcj02(double lng, double lat) {
		if (out_of_china(lng, lat)) {
			return { lng, lat };
		}
		double dlat = transformlat(lng - 105.0, lat - 35.0);
		double dlng = transformlng(lng - 105.0, lat - 35.0);
		double radlat = lat / 180.0 * pi;
		double magic = ::sin(radlat);
		magic = 1 - ee * magic * magic;
		double sqrtmagic = ::sqrt(magic);
		dlat = (dlat * 180.0) / ((a * (1 - ee)) / (magic * sqrtmagic) * pi);
		dlng = (dlng * 180.0) / (a / sqrtmagic * ::cos(radlat) * pi);
		double mglat = lat + dlat;
		double mglng = lng + dlng;
		return { mglng, mglat };
	}

	/**
	 * GCJ02(火星坐标系)转GPS84
	 *
	 * @param lng 火星坐标系的经度
	 * @param lat 火星坐标系纬度
	 * @return WGS84坐标数组
	 */
	static LngLatPair gcj02towgs84(double lng, double lat) {
		if (out_of_china(lng, lat)) {
			return LngLatPair { lng, lat };
		}
		double dlat = transformlat(lng - 105.0, lat - 35.0);
		double dlng = transformlng(lng - 105.0, lat - 35.0);
		double radlat = lat / 180.0 * pi;
		double magic = sin(radlat);
		magic = 1 - ee * magic * magic;
		double sqrtmagic = ::sqrt(magic);
		dlat = (dlat * 180.0) / ((a * (1 - ee)) / (magic * sqrtmagic) * pi);
		dlng = (dlng * 180.0) / (a / sqrtmagic * ::cos(radlat) * pi);
		double mglat = lat + dlat;
		double mglng = lng + dlng;
		return { lng * 2 - mglng, lat * 2 - mglat };
	}

	/**
	 * 纬度转换
	 *
	 * @param lng
	 * @param lat
	 * @return
	 */
	static double transformlat(double lng, double lat) {
		double ret = -100.0 + 2.0 * lng + 3.0 * lat + 0.2 * lat * lat + 0.1 * lng * lat + 0.2 * ::sqrt(::abs(lng));
		ret += (20.0 * ::sin(6.0 * lng * pi) + 20.0 * ::sin(2.0 * lng * pi)) * 2.0 / 3.0;
		ret += (20.0 * ::sin(lat * pi) + 40.0 * ::sin(lat / 3.0 * pi)) * 2.0 / 3.0;
		ret += (160.0 * ::sin(lat / 12.0 * pi) + 320 * ::sin(lat * pi / 30.0)) * 2.0 / 3.0;
		return ret;
	}

	/**
	 * 经度转换
	 *
	 * @param lng
	 * @param lat
	 * @return
	 */
	static double transformlng(double lng, double lat) {
		double ret = 300.0 + lng + 2.0 * lat + 0.1 * lng * lng + 0.1 * lng * lat + 0.1 * ::sqrt(::abs(lng));
		ret += (20.0 * ::sin(6.0 * lng * pi) + 20.0 * ::sin(2.0 * lng * pi)) * 2.0 / 3.0;
		ret += (20.0 * ::sin(lng * pi) + 40.0 * ::sin(lng / 3.0 * pi)) * 2.0 / 3.0;
		ret += (150.0 * ::sin(lng / 12.0 * pi) + 300.0 * ::sin(lng / 30.0 * pi)) * 2.0 / 3.0;
		return ret;
	}

	/**
	 * 判断是否在国内，不在国内不做偏移
	 *
	 * @param lng
	 * @param lat
	 * @return
	 */
	static bool out_of_china(double lng, double lat) {
		if (lng < 72.004 || lng > 137.8347) {
			return true;
		} else if (lat < 0.8293 || lat > 55.8271) {
			return true;
		}
		return false;
	}
};

} //end namespace
#endif
