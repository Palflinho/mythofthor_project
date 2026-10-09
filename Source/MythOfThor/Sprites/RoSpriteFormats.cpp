#include "Sprites/RoSpriteFormats.h"

namespace
{
	/** Leitor sequencial little-endian com checagem de limites. */
	struct FByteReader
	{
		const TArray<uint8>& Data;
		int32 Pos = 0;
		bool bOk = true;

		explicit FByteReader(const TArray<uint8>& InData) : Data(InData) {}

		bool Has(int32 Count) const { return bOk && Count >= 0 && Pos + Count <= Data.Num(); }
		void Skip(int32 Count) { if (Has(Count)) { Pos += Count; } else { bOk = false; } }

		template <typename T>
		T Read()
		{
			T Value{};
			if (Has(sizeof(T)))
			{
				FMemory::Memcpy(&Value, Data.GetData() + Pos, sizeof(T));
				Pos += sizeof(T);
			}
			else
			{
				bOk = false;
			}
			return Value;
		}
	};

	/** Versao dos cabecalhos SPR/ACT: byte [2] = menor, [3] = maior -> 0xMMmm (ex.: 0x205 = 2.5). */
	int32 ReadVersion(FByteReader& R)
	{
		const uint8 Minor = R.Read<uint8>();
		const uint8 Major = R.Read<uint8>();
		return (Major << 8) | Minor;
	}
}

// ------------------------------------------------------------------------------------------------ SPR

bool FRoSpr::LoadPalette(const TArray<uint8>& Bytes, TArray<FColor>& OutPalette)
{
	if (Bytes.Num() < 1024)
	{
		return false;
	}
	const int32 Base = Bytes.Num() - 1024;
	OutPalette.SetNum(256);
	for (int32 i = 0; i < 256; ++i)
	{
		const uint8* P = Bytes.GetData() + Base + i * 4;
		OutPalette[i] = FColor(P[0], P[1], P[2], i == 0 ? 0 : 255); // indice 0 = transparente
	}
	return true;
}

bool FRoSpr::Load(const TArray<uint8>& Bytes, const TArray<FColor>* PaletteOverride)
{
	Indexed.Reset();
	Rgba.Reset();
	Palette.Reset();

	FByteReader R(Bytes);
	if (!R.Has(6) || Bytes[0] != 'S' || Bytes[1] != 'P')
	{
		return false;
	}
	R.Skip(2);
	const int32 Version = ReadVersion(R);
	const int32 IndexedCount = R.Read<uint16>();
	const int32 RgbaCount = Version > 0x101 ? R.Read<uint16>() : 0;

	// A paleta fica nos ultimos 1024 bytes (versao > 1.0).
	if (PaletteOverride && PaletteOverride->Num() == 256)
	{
		Palette = *PaletteOverride;
	}
	else if (Version > 0x100)
	{
		LoadPalette(Bytes, Palette);
	}
	else
	{
		Palette.Init(FColor::Magenta, 256);
	}
	Palette[0].A = 0;

	Indexed.SetNum(IndexedCount);
	for (FRoSprImage& Img : Indexed)
	{
		Img.Width = R.Read<uint16>();
		Img.Height = R.Read<uint16>();
		const int32 Total = Img.Width * Img.Height;
		Img.Pixels.SetNumZeroed(Total);
		if (Version >= 0x201)
		{
			// RLE: 0x00 seguido da quantidade de pixels transparentes
			const int32 Size = R.Read<uint16>();
			if (!R.Has(Size))
			{
				return false;
			}
			const uint8* Src = Bytes.GetData() + R.Pos;
			int32 Out = 0;
			for (int32 i = 0; i < Size && Out < Total; ++i)
			{
				const uint8 Index = Src[i];
				if (Index == 0 && i + 1 < Size)
				{
					const int32 Run = Src[++i];
					for (int32 k = 0; k < Run && Out < Total; ++k)
					{
						Img.Pixels[Out++] = Palette[0];
					}
				}
				else
				{
					Img.Pixels[Out++] = Palette[Index];
				}
			}
			R.Skip(Size);
		}
		else
		{
			if (!R.Has(Total))
			{
				return false;
			}
			for (int32 i = 0; i < Total; ++i)
			{
				Img.Pixels[i] = Palette[Bytes[R.Pos + i]];
			}
			R.Skip(Total);
		}
	}

	Rgba.SetNum(RgbaCount);
	for (FRoSprImage& Img : Rgba)
	{
		Img.Width = R.Read<uint16>();
		Img.Height = R.Read<uint16>();
		const int32 Total = Img.Width * Img.Height;
		if (!R.Has(Total * 4))
		{
			return false;
		}
		Img.Pixels.SetNumZeroed(Total);
		// Pixels em ABGR, linhas de baixo para cima.
		for (int32 y = 0; y < Img.Height; ++y)
		{
			const uint8* Row = Bytes.GetData() + R.Pos + (Img.Height - 1 - y) * Img.Width * 4;
			for (int32 x = 0; x < Img.Width; ++x)
			{
				const uint8* P = Row + x * 4;
				Img.Pixels[y * Img.Width + x] = FColor(P[3], P[2], P[1], P[0]);
			}
		}
		R.Skip(Total * 4);
	}
	return R.bOk;
}

const FRoSprImage* FRoSpr::Find(int32 Index, int32 SprType) const
{
	const TArray<FRoSprImage>& List = SprType == 1 ? Rgba : Indexed;
	return List.IsValidIndex(Index) ? &List[Index] : nullptr;
}

// ------------------------------------------------------------------------------------------------ ACT

bool FRoAct::Load(const TArray<uint8>& Bytes)
{
	Actions.Reset();
	Events.Reset();

	FByteReader R(Bytes);
	if (!R.Has(16) || Bytes[0] != 'A' || Bytes[1] != 'C')
	{
		return false;
	}
	R.Skip(2);
	Version = ReadVersion(R);
	const int32 ActionCount = R.Read<uint16>();
	R.Skip(10);

	Actions.SetNum(ActionCount);
	for (FRoActAction& Action : Actions)
	{
		const int32 FrameCount = R.Read<uint32>();
		if (!R.bOk || FrameCount > 10000)
		{
			return false;
		}
		Action.Frames.SetNum(FrameCount);
		for (FRoActFrame& Frame : Action.Frames)
		{
			R.Skip(32); // retangulos de area (nao usados)
			const int32 LayerCount = R.Read<uint32>();
			if (!R.bOk || LayerCount > 1000)
			{
				return false;
			}
			Frame.Layers.SetNum(LayerCount);
			for (FRoActLayer& L : Frame.Layers)
			{
				L.Offset.X = R.Read<int32>();
				L.Offset.Y = R.Read<int32>();
				L.SprIndex = R.Read<int32>();
				L.bMirror = R.Read<int32>() != 0;
				if (Version >= 0x200)
				{
					const uint8 Cr = R.Read<uint8>(), Cg = R.Read<uint8>(), Cb = R.Read<uint8>(), Ca = R.Read<uint8>();
					L.Color = FColor(Cr, Cg, Cb, Ca);
					L.Scale.X = R.Read<float>();
					L.Scale.Y = Version >= 0x204 ? R.Read<float>() : L.Scale.X;
					L.RotationDeg = R.Read<int32>();
					L.SprType = R.Read<int32>();
					if (Version >= 0x205)
					{
						R.Skip(8); // largura/altura (redundante com o .spr)
					}
				}
			}
			Frame.EventId = Version >= 0x200 ? R.Read<int32>() : -1;
			if (Version >= 0x203)
			{
				const int32 AnchorCount = R.Read<int32>();
				if (!R.bOk || AnchorCount < 0 || AnchorCount > 100)
				{
					return false;
				}
				Frame.Anchors.SetNum(AnchorCount);
				for (FIntPoint& A : Frame.Anchors)
				{
					R.Skip(4);
					A.X = R.Read<int32>();
					A.Y = R.Read<int32>();
					R.Skip(4);
				}
			}
		}
	}

	if (Version >= 0x201)
	{
		const int32 EventCount = R.Read<int32>();
		for (int32 i = 0; i < EventCount && R.Has(40); ++i)
		{
			const ANSICHAR* Name = reinterpret_cast<const ANSICHAR*>(Bytes.GetData() + R.Pos);
			Events.Add(FString(FCStringAnsi::Strnlen(Name, 40), Name));
			R.Skip(40);
		}
	}
	if (Version >= 0x202)
	{
		for (FRoActAction& Action : Actions)
		{
			// O arquivo guarda o intervalo em unidades de 25 ms.
			Action.IntervalMs = R.Read<float>() * 25.f;
		}
	}
	else
	{
		for (FRoActAction& Action : Actions)
		{
			Action.IntervalMs = 150.f;
		}
	}
	return R.bOk;
}
