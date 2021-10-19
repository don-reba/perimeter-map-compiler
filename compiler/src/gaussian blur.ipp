//-------------------------------------------------------------------
// fast 5x5 Gaussian blur by Frederick M. Waltz and John W. V. Miller
// http://www-personal.engin.umd.umich.edu/~jwvm/ece581/21_GBlur.pdf
// does not blur the 3-pixel border around the image
//-------------------------------------------------------------------

// TODO: calculate how large the state variables get
// TODO: write a metaprogram to create blurs with arbitrary matrices

#pragma once

template <typename T, typename StateT = T>
class GaussianBlurT
{
private:
	struct SC {
		SC() : sc0(), sc1(), sc2(), sc3() {}
		StateT sc0, sc1, sc2, sc3;
	};
public:
	static void Apply(T *pix, SIZE size)
	{
		StateT sr0, sr1, sr2, sr3; // row state
		vector<SC> sc(size.cx);    // column state
		const LONG write_offset(-size.cx * 2 - 2);
		for (LONG y = 0; y != 4; ++y)
		{
			sr3 = sr2 = sr1 = sr0 = 0;
			for (LONG x = 0; x != size.cx; ++x)
			{
				StateT &sc0(sc[x].sc0), &sc1(sc[x].sc1), &sc2(sc[x].sc2), &sc3(sc[x].sc3);
				StateT tmp1 = *pix;
				StateT tmp2 = sr0 + tmp1;
				sr0 = tmp1;
				tmp1 = sr1 + tmp2;
				sr1 = tmp2;
				tmp2 = sr2 + tmp1;
				sr2 = tmp1;
				tmp1 = sr3 + tmp2;
				sr3 = tmp2;
				tmp2 = sc0 + tmp1;
				sc0 = tmp1;
				tmp1 = sc1 + tmp2;
				sc1 = tmp2;
				tmp2 = sc2 + tmp1;
				sc2 = tmp1;
				sc3 = tmp2;
				++pix;
			}
		}
		for (LONG y = 4; y != size.cy; ++y)
		{
			sr3 = sr2 = sr1 = sr0 = 0;
			for (LONG x = 0; x != 4; ++x)
			{
				StateT &sc0(sc[x].sc0), &sc1(sc[x].sc1), &sc2(sc[x].sc2), &sc3(sc[x].sc3);
				StateT tmp1 = *pix;
				StateT tmp2 = sr0 + tmp1;
				sr0 = tmp1;
				tmp1 = sr1 + tmp2;
				sr1 = tmp2;
				tmp2 = sr2 + tmp1;
				sr2 = tmp1;
				tmp1 = sr3 + tmp2;
				sr3 = tmp2;
				tmp2 = sc0 + tmp1;
				sc0 = tmp1;
				tmp1 = sc1 + tmp2;
				sc1 = tmp2;
				tmp2 = sc2 + tmp1;
				sc2 = tmp1;
				sc3 = tmp2;
				++pix;
			}
			for (LONG x = 4; x != size.cx; ++x)
			{
				StateT &sc0(sc[x].sc0), &sc1(sc[x].sc1), &sc2(sc[x].sc2), &sc3(sc[x].sc3);
				StateT tmp1 = *pix;
				StateT tmp2 = sr0 + tmp1;
				sr0 = tmp1;
				tmp1 = sr1 + tmp2;
				sr1 = tmp2;
				tmp2 = sr2 + tmp1;
				sr2 = tmp1;
				tmp1 = sr3 + tmp2;
				sr3 = tmp2;
				tmp2 = sc0 + tmp1;
				sc0 = tmp1;
				tmp1 = sc1 + tmp2;
				sc1 = tmp2;
				tmp2 = sc2 + tmp1;
				sc2 = tmp1;
				pix[write_offset] = static_cast<T>((static_cast<StateT>(0x80) + sc3 + tmp2) / static_cast<StateT>(0x100));
				sc3 = tmp2;
				++pix;
			}
		}
	}
};

template <typename T>
void GaussianBlur(T *pix, SIZE size)
{
	GaussianBlurT<T>::Apply(pix, size);
}

template <typename T, typename StateT>
void GaussianBlur(T *pix, SIZE size)
{
	GaussianBlurT<T, StateT>::Apply(pix, size);
}