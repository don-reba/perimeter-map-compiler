using System;
using System.Runtime.InteropServices;

namespace TriggerEdit
{
	public class Import
	{
		public enum BeepType
		{          
			SimpleBeep   = -1,
			OK           = 0x00,
			Question     = 0x20,
			Exclamation  = 0x30,
			Asterisk     = 0x40,
		}

		[DllImport("User32.dll", ExactSpelling=true)]
		private static extern bool MessageBeep(uint type);

		public static void Beep(BeepType type)
		{
			MessageBeep((uint)type);
		}

		[StructLayout(LayoutKind.Sequential)]
			public struct Message
		{
			public IntPtr hWnd;
			public uint   msg;
			public IntPtr wParam;
			public IntPtr lParam;
			public uint   time;
			public System.Drawing.Point p;
		}

		[System.Security.SuppressUnmanagedCodeSecurity] // We won't use this maliciously
		[DllImport("User32.dll", CharSet=CharSet.Auto)]
		public static extern bool PeekMessage(out Message msg, IntPtr hWnd, uint messageFilterMin, uint messageFilterMax, uint flags);

		public enum VK 
		{
			SHIFT             = 0x10,
			CONTROL           = 0x11,
			MENU              = 0x12,
			ESCAPE            = 0x1B,
			BACK              = 0x08,
			TAB               = 0x09,
			RETURN            = 0x0D,
			SPACE             = 0x20,
			PRIOR             = 0x21,
			NEXT              = 0x22,
			END               = 0x23,
			HOME              = 0x24,
			LEFT              = 0x25,
			UP                = 0x26,
			RIGHT             = 0x27,
			DOWN              = 0x28,
			SELECT            = 0x29,
			PRINT             = 0x2A,
			EXECUTE           = 0x2B,
			SNAPSHOT          = 0x2C,
			INSERT            = 0x2D,
			DELETE            = 0x2E,
			HELP              = 0x2F,
			NUM_0             = 0x30,
			NUM_1             = 0x31,
			NUM_2             = 0x32,
			NUM_3             = 0x33,
			NUM_4             = 0x34,
			NUM_5             = 0x35,
			NUM_6             = 0x36,
			NUM_7             = 0x37,
			NUM_8             = 0x38,
			NUM_9             = 0x39,
			ALPHA_A           = 0x41,
			ALPHA_B           = 0x42,
			ALPHA_C           = 0x43,
			ALPHA_D           = 0x44,
			ALPHA_E           = 0x45,
			ALPHA_F           = 0x46,
			ALPHA_G           = 0x47,
			ALPHA_H           = 0x48,
			ALPHA_I           = 0x49,
			ALPHA_J           = 0x4A,
			ALPHA_K           = 0x4B,
			ALPHA_L           = 0x4C,
			ALPHA_M           = 0x4D,
			ALPHA_N           = 0x4E,
			ALPHA_O           = 0x4F,
			ALPHA_P           = 0x50,
			ALPHA_Q           = 0x51,
			ALPHA_R           = 0x52,
			ALPHA_S           = 0x53,
			ALPHA_T           = 0x54,
			ALPHA_U           = 0x55,
			ALPHA_V           = 0x56,
			ALPHA_W           = 0x57,
			ALPHA_X           = 0x58,
			ALPHA_Y           = 0x59,
			ALPHA_Z           = 0x5A,
			NUMPAD0           = 0x60,
			NUMPAD1           = 0x61,
			NUMPAD2           = 0x62,
			NUMPAD3           = 0x63,
			NUMPAD4           = 0x64,
			NUMPAD5           = 0x65,
			NUMPAD6           = 0x66,
			NUMPAD7           = 0x67,
			NUMPAD8           = 0x68,
			NUMPAD9           = 0x69,
			MULTIPLY          = 0x6A,
			ADD               = 0x6B,
			SEPARATOR         = 0x6C,
			SUBTRACT          = 0x6D,
			DECIMAL           = 0x6E,
			DIVIDE            = 0x6F,
			F1                = 0x70,
			F2                = 0x71,
			F3                = 0x72,
			F4                = 0x73,
			F5                = 0x74,
			F6                = 0x75,
			F7                = 0x76,
			F8                = 0x77,
			F9                = 0x78,
			F10               = 0x79,
			F11               = 0x7A,
			F12               = 0x7B,
			OEM_1             = 0xBA,   // ',:' for US
			OEM_PLUS          = 0xBB,   // '+' any country
			OEM_COMMA         = 0xBC,   // ',' any country
			OEM_MINUS         = 0xBD,   // '-' any country
			OEM_PERIOD        = 0xBE,   // '.' any country
			OEM_2             = 0xBF,   // '/?' for US
			OEM_3             = 0xC0,   // '`~' for US
			MEDIA_NEXT_TRACK  = 0xB0,
			MEDIA_PREV_TRACK  = 0xB1,
			MEDIA_STOP        = 0xB2,
			MEDIA_PLAY_PAUSE  = 0xB3,
			LWIN              = 0x5B,
			RWIN              = 0x5C
		}

		public enum WindowsMessages
		{
			WM_ACTIVATE = 0x6,
			WM_ACTIVATEAPP = 0x1C,
			WM_AFXFIRST = 0x360,
			WM_AFXLAST = 0x37F,
			WM_APP = 0x8000,
			WM_ASKCBFORMATNAME = 0x30C,
			WM_CANCELJOURNAL = 0x4B,
			WM_CANCELMODE = 0x1F,
			WM_CAPTURECHANGED = 0x215,
			WM_CHANGECBCHAIN = 0x30D,
			WM_CHAR = 0x102,
			WM_CHARTOITEM = 0x2F,
			WM_CHILDACTIVATE = 0x22,
			WM_CLEAR = 0x303,
			WM_CLOSE = 0x10,
			WM_COMMAND = 0x111,
			WM_COMPACTING = 0x41,
			WM_COMPAREITEM = 0x39,
			WM_CONTEXTMENU = 0x7B,
			WM_COPY = 0x301,
			WM_COPYDATA = 0x4A,
			WM_CREATE = 0x1,
			WM_CTLCOLORBTN = 0x135,
			WM_CTLCOLORDLG = 0x136,
			WM_CTLCOLOREDIT = 0x133,
			WM_CTLCOLORLISTBOX = 0x134,
			WM_CTLCOLORMSGBOX = 0x132,
			WM_CTLCOLORSCROLLBAR = 0x137,
			WM_CTLCOLORSTATIC = 0x138,
			WM_CUT = 0x300,
			WM_DEADCHAR = 0x103,
			WM_DELETEITEM = 0x2D,
			WM_DESTROY = 0x2,
			WM_DESTROYCLIPBOARD = 0x307,
			WM_DEVICECHANGE = 0x219,
			WM_DEVMODECHANGE = 0x1B,
			WM_DISPLAYCHANGE = 0x7E,
			WM_DRAWCLIPBOARD = 0x308,
			WM_DRAWITEM = 0x2B,
			WM_DROPFILES = 0x233,
			WM_ENABLE = 0xA,
			WM_ENDSESSION = 0x16,
			WM_ENTERIDLE = 0x121,
			WM_ENTERMENULOOP = 0x211,
			WM_ENTERSIZEMOVE = 0x231,
			WM_ERASEBKGND = 0x14,
			WM_EXITMENULOOP = 0x212,
			WM_EXITSIZEMOVE = 0x232,
			WM_FONTCHANGE = 0x1D,
			WM_GETDLGCODE = 0x87,
			WM_GETFONT = 0x31,
			WM_GETHOTKEY = 0x33,
			WM_GETICON = 0x7F,
			WM_GETMINMAXINFO = 0x24,
			WM_GETOBJECT = 0x3D,
			WM_GETTEXT = 0xD,
			WM_GETTEXTLENGTH = 0xE,
			WM_HANDHELDFIRST = 0x358,
			WM_HANDHELDLAST = 0x35F,
			WM_HELP = 0x53,
			WM_HOTKEY = 0x312,
			WM_HSCROLL = 0x114,
			WM_HSCROLLCLIPBOARD = 0x30E,
			WM_ICONERASEBKGND = 0x27,
			WM_IME_CHAR = 0x286,
			WM_IME_COMPOSITION = 0x10F,
			WM_IME_COMPOSITIONFULL = 0x284,
			WM_IME_CONTROL = 0x283,
			WM_IME_ENDCOMPOSITION = 0x10E,
			WM_IME_KEYDOWN = 0x290,
			WM_IME_KEYLAST = 0x10F,
			WM_IME_KEYUP = 0x291,
			WM_IME_NOTIFY = 0x282,
			WM_IME_REQUEST = 0x288,
			WM_IME_SELECT = 0x285,
			WM_IME_SETCONTEXT = 0x281,
			WM_IME_STARTCOMPOSITION = 0x10D,
			WM_INITDIALOG = 0x110,
			WM_INITMENU = 0x116,
			WM_INITMENUPOPUP = 0x117,
			WM_INPUTLANGCHANGE = 0x51,
			WM_INPUTLANGCHANGEREQUEST = 0x50,
			WM_KEYDOWN = 0x100,
			WM_KEYFIRST = 0x100,
			WM_KEYLAST = 0x108,
			WM_KEYUP = 0x101,
			WM_KILLFOCUS = 0x8,
			WM_LBUTTONDBLCLK = 0x203,
			WM_LBUTTONDOWN = 0x201,
			WM_LBUTTONUP = 0x202,
			WM_MBUTTONDBLCLK = 0x209,
			WM_MBUTTONDOWN = 0x207,
			WM_MBUTTONUP = 0x208,
			WM_MDIACTIVATE = 0x222,
			WM_MDICASCADE = 0x227,
			WM_MDICREATE = 0x220,
			WM_MDIDESTROY = 0x221,
			WM_MDIGETACTIVE = 0x229,
			WM_MDIICONARRANGE = 0x228,
			WM_MDIMAXIMIZE = 0x225,
			WM_MDINEXT = 0x224,
			WM_MDIREFRESHMENU = 0x234,
			WM_MDIRESTORE = 0x223,
			WM_MDISETMENU = 0x230,
			WM_MDITILE = 0x226,
			WM_MEASUREITEM = 0x2C,
			WM_MENUCHAR = 0x120,
			WM_MENUCOMMAND = 0x126,
			WM_MENUDRAG = 0x123,
			WM_MENUGETOBJECT = 0x124,
			WM_MENURBUTTONUP = 0x122,
			WM_MENUSELECT = 0x11F,
			WM_MOUSEACTIVATE = 0x21,
			WM_MOUSEFIRST = 0x200,
			WM_MOUSEHOVER = 0x2A1,
			WM_MOUSELAST = 0x20A,
			WM_MOUSELEAVE = 0x2A3,
			WM_MOUSEMOVE = 0x200,
			WM_MOUSEWHEEL = 0x20A,
			WM_MOVE = 0x3,
			WM_MOVING = 0x216,
			WM_NCACTIVATE = 0x86,
			WM_NCCALCSIZE = 0x83,
			WM_NCCREATE = 0x81,
			WM_NCDESTROY = 0x82,
			WM_NCHITTEST = 0x84,
			WM_NCLBUTTONDBLCLK = 0xA3,
			WM_NCLBUTTONDOWN = 0xA1,
			WM_NCLBUTTONUP = 0xA2,
			WM_NCMBUTTONDBLCLK = 0xA9,
			WM_NCMBUTTONDOWN = 0xA7,
			WM_NCMBUTTONUP = 0xA8,
			WM_NCMOUSEHOVER = 0x2A0,
			WM_NCMOUSELEAVE = 0x2A2,
			WM_NCMOUSEMOVE = 0xA0,
			WM_NCPAINT = 0x85,
			WM_NCRBUTTONDBLCLK = 0xA6,
			WM_NCRBUTTONDOWN = 0xA4,
			WM_NCRBUTTONUP = 0xA5,
			WM_NEXTDLGCTL = 0x28,
			WM_NEXTMENU = 0x213,
			WM_NOTIFY = 0x4E,
			WM_NOTIFYFORMAT = 0x55,
			WM_NULL = 0x0,
			WM_PAINT = 0xF,
			WM_PAINTCLIPBOARD = 0x309,
			WM_PAINTICON = 0x26,
			WM_PALETTECHANGED = 0x311,
			WM_PALETTEISCHANGING = 0x310,
			WM_PARENTNOTIFY = 0x210,
			WM_PASTE = 0x302,
			WM_PENWINFIRST = 0x380,
			WM_PENWINLAST = 0x38F,
			WM_POWER = 0x48,
			WM_PRINT = 0x317,
			WM_PRINTCLIENT = 0x318,
			WM_QUERYDRAGICON = 0x37,
			WM_QUERYENDSESSION = 0x11,
			WM_QUERYNEWPALETTE = 0x30F,
			WM_QUERYOPEN = 0x13,
			WM_QUEUESYNC = 0x23,
			WM_QUIT = 0x12,
			WM_RBUTTONDBLCLK = 0x206,
			WM_RBUTTONDOWN = 0x204,
			WM_RBUTTONUP = 0x205,
			WM_RENDERALLFORMATS = 0x306,
			WM_RENDERFORMAT = 0x305,
			WM_SETCURSOR = 0x20,
			WM_SETFOCUS = 0x7,
			WM_SETFONT = 0x30,
			WM_SETHOTKEY = 0x32,
			WM_SETICON = 0x80,
			WM_SETREDRAW = 0xB,
			WM_SETTEXT = 0xC,
			WM_SETTINGCHANGE = 0x1A,
			WM_SHOWWINDOW = 0x18,
			WM_SIZE = 0x5,
			WM_SIZECLIPBOARD = 0x30B,
			WM_SIZING = 0x214,
			WM_SPOOLERSTATUS = 0x2A,
			WM_STYLECHANGED = 0x7D,
			WM_STYLECHANGING = 0x7C,
			WM_SYNCPAINT = 0x88,
			WM_SYSCHAR = 0x106,
			WM_SYSCOLORCHANGE = 0x15,
			WM_SYSCOMMAND = 0x112,
			WM_SYSDEADCHAR = 0x107,
			WM_SYSKEYDOWN = 0x104,
			WM_SYSKEYUP = 0x105,
			WM_TCARD = 0x52,
			WM_TIMECHANGE = 0x1E,
			WM_TIMER = 0x113,
			WM_UNDO = 0x304,
			WM_UNINITMENUPOPUP = 0x125,
			WM_USER = 0x400,
			WM_USERCHANGED = 0x54,
			WM_VKEYTOITEM = 0x2E,
			WM_VSCROLL = 0x115,
			WM_VSCROLLCLIPBOARD = 0x30A,
			WM_WINDOWPOSCHANGED = 0x47,
			WM_WINDOWPOSCHANGING = 0x46,
			WM_WININICHANGE = 0x1A
		}

		[StructLayout( LayoutKind.Sequential )]
		public struct POINT 
		{
			public int X;
			public int Y;

			public POINT( int x, int y ) 
			{
				this.X = x;
				this.Y = y;
			}

			public static implicit operator System.Drawing.Point( POINT p ) 
			{
				return new System.Drawing.Point( p.X,  p.Y );
			}

			public static implicit operator POINT( System.Drawing.Point p ) 
			{
				return new POINT( p.X, p.Y );
			}
		}

		[DllImport("user32.dll")]
		public static extern IntPtr WindowFromPoint(POINT point);

		[DllImport("user32.dll")]
		public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

		[DllImport("user32.dll", CharSet=CharSet.Auto)]
		public static extern int SendMessage(IntPtr hWnd, int msg, int wParam, ref POINT lParam);
	}
}
