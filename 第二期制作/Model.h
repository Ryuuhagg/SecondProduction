//Model.h
#pragma once

struct ModelData {
	const char* path;
	int idle;
	int walk;
	int jump;
	int dash;
};
extern ModelData beaver;
extern ModelData cat;
extern ModelData bee;
extern ModelData bunny;