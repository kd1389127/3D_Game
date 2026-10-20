#include "KdEasing.h"

float KdEase::InSine(float _t)
{
	return 1.0f - std::cos(_t * DirectX::XM_PI * 0.5f);
}

float KdEase::OutSine(float _t)
{
	return std::sin(_t * DirectX::XM_PI * 0.5f);
}

float KdEase::InOutSine(float _t)
{
	return -(std::cos(DirectX::XM_PI * _t) - 1.0f) * 0.5f;
}

float KdEase::OutCubic(float _t)
{
	return 1.0f - std::pow(1.0f - _t, 3.0f);
}

float KdEase::OutBounce(float _t)
{
	const float n1 = 7.5625f;
	const float d1 = 2.75f;

	if (_t < 1.0f / d1)
	{
		return n1 * _t * _t;
	}
	else if (_t < 2.0f / d1)
	{
		_t -= 1.5f / d1;	// 元は「引数を書き換えながら同じ行で使う」書き方で危ないので、行を分けた
		return n1 * _t * _t + 0.75f;
	}
	else if (_t < 2.5f / d1)
	{
		_t -= 2.25f / d1;
		return n1 * _t * _t + 0.9375f;
	}
	else
	{
		_t -= 2.625f / d1;
		return n1 * _t * _t + 0.984375f;
	}
}