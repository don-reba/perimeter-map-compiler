#pragma once

class CViewer
{
public:
	// view window serialization attributes
	struct WndSaveInfo
	{
		string name;
		RECT   rect;
		bool   is_visible;
	};
public:
	virtual void        Update(int caller) = 0;
	virtual void        ToggleVisibility(bool show) = 0;
	virtual WndSaveInfo GetSaveInfo() = 0;
};