#pragma once

#define FILE_INI_CONFIG	"config.ini"

class CConfig
{
public:
	void Init();
	void Load();

	float GetDelay() const { return m_DelayExec; };

private:
	void ResetValues();

private:
	char m_PathDir[MAX_PATH_LENGTH];

	// settings
	float m_DelayExec;
};

template <typename T>
T clamp(T a, T min, T max)	{ return (a > max) ? max : (a < min) ? min : a; }

extern CConfig Config;
