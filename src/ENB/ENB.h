#pragma once

#include "ENBSeriesAPI.h"

namespace ENB
{
	inline ENB_API::ENBSDKALT1001* handle{ nullptr };

	template <class T>
	T GetParameter(const char* a_filename, const char* a_category, const char* a_keyname)
	{
		ENBParameter param;
		if constexpr (std::is_same_v<T, bool>) {
			BOOL bvalue;
			if (handle->GetParameter(a_filename, a_category, a_keyname, &param)) {
				if (param.Type == ENBParameterType::ENBParam_BOOL) {
					std::memcpy(&bvalue, param.Data, ENBParameterTypeToSize(ENBParameterType::ENBParam_BOOL));
					return bvalue;
				}
			}
		} else if constexpr (std::is_same_v<T, float>) {
			float fvalue;
			if (handle->GetParameter(a_filename, a_category, a_keyname, &param)) {
				if (param.Type == ENB_SDK::ENBParameterType::ENBParam_FLOAT) {
					memcpy(&fvalue, param.Data, ENBParameterTypeToSize(ENBParameterType::ENBParam_FLOAT));
					return fvalue;
				}
			}
		}
		return {};
	}
	template <class T>
	bool SetParameter(T a_value, const char* a_filename, const char* a_category, const char* a_keyname)
	{
		ENBParameter param{};
		if constexpr (std::is_same_v<T, bool>) {
			param.Type = ENBParameterType::ENBParam_BOOL;
		} else if constexpr (std::is_same_v<T, float>) {
			param.Type = ENBParameterType::ENBParam_FLOAT;
		}
		param.Size = ENBParameterTypeToSize(param.Type);
		std::memcpy(param.Data, &a_value, param.Size);

		return handle->SetParameter(a_filename, a_category, a_keyname, &param);
	}

	inline bool IsInstalled()
	{
		return handle != nullptr;
	}

    inline bool IsEnabled()
	{
		return IsInstalled() && GetParameter<bool>("enbseries.ini", "GLOBAL", "UseEffect");
	}
}
