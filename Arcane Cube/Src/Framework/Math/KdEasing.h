#pragma once

// 10月20日フレームワーク内を改造
// イージング関数。_t は進行度(0=開始 1=終了)
// ・inlineは付けない：inline関数は、使う側のcppにも中身が見えている必要があるため
//   (元はinlineなのに中身がcppにあったので、他のcppから呼ぶとリンクエラーだった)
// ・staticを付ける：KdEase::OutSine(t) のように、オブジェクトを作らずに呼べる
struct KdEase
{
	static float InSine(float _t);
	static float OutSine(float _t);
	static float InOutSine(float _t);

	static float OutCubic(float _t);	// メニューの開閉に使う

	static float OutBounce(float _t);
};