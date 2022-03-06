#pragma once
#define _DEBUG

#ifdef _DEBUG
#define PRAGMA_OPTION PRAGMA_DISABLE_OPTIMIZATION

#else
#define PRAGMA_OPTION
#endif


const float kDropEmitChanceDefault = 0.01f;
const float kDropEmitRadiusMinDefault = 1.5f;
const float kDropEmitRadiusMaxDefault = 4.0f;
const float kDropEmitRadiusExpDefault = 2.0f;

const float kDropEmitChanceStrokeEnd = 1.0f;
const float kDropEmitRadiusMinStrokeEnd = 6.0f;
const float kDropEmitRadiusMaxStrokeEnd = 9.0f;
const float kDropEmitRadiusExpStrokeEnd = 0.5f;