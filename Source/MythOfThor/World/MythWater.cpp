#include "World/MythWater.h"

#include "World/MythMapInfo.h"
#include "UI/MythOptions.h"
#include "EngineUtils.h"
#include "UI/MythRoUITextures.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AMythWater::AMythWater()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

AMythWater* AMythWater::SpawnForMap(UWorld* World)
{
	AMythMapInfo* Info = AMythMapInfo::Find(World);
	if (!World || !Info)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMythWater* Water = World->SpawnActor<AMythWater>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (Water && !Water->Build(Info->MapName.ToLower(), Info->CellOriginUU))
	{
		Water->Destroy();
		return nullptr;
	}
	return Water;
}

bool AMythWater::Build(const FString& MapName, const FVector2D& Origin)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("water"), MapName + TEXT(".json"))))
	{
		return false; // mapa sem agua
	}
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}
	const float Level = static_cast<float>(Root->GetNumberField(TEXT("level")));
	const int32 Type = Root->GetIntegerField(TEXT("type"));
	AnimSpeed = FMath::Max(1.f, static_cast<float>(Root->GetNumberField(TEXT("anim"))));
	const float TileUU = static_cast<float>(Root->GetNumberField(TEXT("scale"))) * 10.f; // tile do GND (10) * escala do importador
	const float Z = -Level * 10.f;

	for (int32 i = 0; i < 32; ++i)
	{
		UTexture2D* Tex = FMythRoUITextures::Get(FString::Printf(TEXT("../워터/water%d%02d.jpg"), Type, i));
		if (Tex) { Frames.Add(Tex); }
	}
	if (Frames.Num() == 0)
	{
		return false;
	}

	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	const TSharedPtr<FJsonObject>* Rows = nullptr;
	if (!Root->TryGetObjectField(TEXT("rows"), Rows) || !Rows)
	{
		return false;
	}
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Row : (*Rows)->Values)
	{
		const int32 Ty = FCString::Atoi(*Row.Key);
		const TArray<TSharedPtr<FJsonValue>>& Runs = Row.Value->AsArray();
		for (int32 i = 0; i + 1 < Runs.Num(); i += 2)
		{
			const int32 X0 = static_cast<int32>(Runs[i]->AsNumber());
			const int32 Len = static_cast<int32>(Runs[i + 1]->AsNumber());
			const float WX0 = Origin.X + X0 * TileUU;
			const float WX1 = Origin.X + (X0 + Len) * TileUU;
			const float WY0 = Origin.Y - Ty * TileUU;       // y cresce para -Y no mundo
			const float WY1 = Origin.Y - (Ty + 1) * TileUU;
			const int32 Base0 = Verts.Num();
			Verts.Add(FVector(WX0, WY0, Z));
			Verts.Add(FVector(WX1, WY0, Z));
			Verts.Add(FVector(WX1, WY1, Z));
			Verts.Add(FVector(WX0, WY1, Z));
			// a textura se repete a cada 5 tiles, como o cliente
			UVs.Add(FVector2D(X0 / 5.f, Ty / 5.f));
			UVs.Add(FVector2D((X0 + Len) / 5.f, Ty / 5.f));
			UVs.Add(FVector2D((X0 + Len) / 5.f, (Ty + 1) / 5.f));
			UVs.Add(FVector2D(X0 / 5.f, (Ty + 1) / 5.f));
			for (int32 k = 0; k < 4; ++k) { Normals.Add(FVector::UpVector); }
			// duas faces (de cima e de baixo): a camera do jogo olha de cima, mas o plano nao pode sumir de baixo
			Tris.Append({ Base0, Base0 + 1, Base0 + 2, Base0, Base0 + 2, Base0 + 3 });
			Tris.Append({ Base0, Base0 + 2, Base0 + 1, Base0, Base0 + 3, Base0 + 2 });
		}
	}
	if (Verts.Num() == 0)
	{
		return false;
	}
	Mesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	BaseOpacity = (Type == 4 || Type == 6) ? 1.f : 0.78f;
	ApplyStyle(); // material depois do CreateMeshSection: o colocado antes se perde	UE_LOG(LogTemp, Log, TEXT("MythWater: %s water level %.1f type %d, %d blocks"), *MapName, Level, Type, Verts.Num() / 4);
	return true;
}

void AMythWater::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (!Material || Frames.Num() == 0)
	{
		return;
	}
	// 60 quadros por segundo de relogio dividido por animSpeed, como o cliente (frame / animSpeed) % 32
	const int32 Frame = (static_cast<int32>(Time * 60.f / AnimSpeed)) % Frames.Num();
	if (Frame != LastFrame)
	{
		LastFrame = Frame;
		Material->SetTextureParameterValue(TEXT("Tex"), Frames[Frame]);
	}
}

void AMythWater::ApplyStyle()
{
	const bool bModern = FMythOptions::Get(TEXT("ModernWater"));
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bModern ? TEXT("/Game/Myth/M_WaterModern.M_WaterModern") : TEXT("/Game/Myth/M_Water.M_Water"));
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Myth/M_Water.M_Water"));
	}
	if (!Base || Frames.Num() == 0)
	{
		return;
	}
	Material = UMaterialInstanceDynamic::Create(Base, this);
	Material->SetScalarParameterValue(TEXT("Opacity"), bModern ? 0.9f : BaseOpacity);
	Material->SetScalarParameterValue(TEXT("Brightness"), 1.1f);
	Material->SetTextureParameterValue(TEXT("Tex"), Frames[FMath::Clamp(LastFrame, 0, Frames.Num() - 1)]);
	Mesh->SetMaterial(0, Material);
	if (LastFrame < 0) { LastFrame = 0; }
}

void AMythWater::ApplyStyleToAll(UWorld* World)
{
	if (!World)
	{
		return;
	}
	for (TActorIterator<AMythWater> It(World); It; ++It)
	{
		It->ApplyStyle();
	}
}
