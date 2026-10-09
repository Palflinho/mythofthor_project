#include "Sprites/MythRoSpriteComponent.h"

#include "World/MythDayNightActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	// Caminhos do GRF em coreano, escritos como codigos Unicode para nao depender da codificacao do fonte.
	const TCHAR* HumanDir = TEXT("인간족");   // 인간족
	const TCHAR* BodyDir = TEXT("몸통");          // 몸통
	const TCHAR* HeadDir = TEXT("머리통");    // 머리통
	const TCHAR* HairPalDir = TEXT("머리");       // 머리
	const TCHAR* MaleTag = TEXT("남");                // 남
	const TCHAR* FemaleTag = TEXT("여");              // 여
	const TCHAR* MonsterDir = TEXT("몬스터"); // 몬스터

	// Grupos de acao do .act (acao = grupo * 8 + direcao), iguais ao cliente original.
	// Jogador: 0 parado, 1 andando, 2 sentado, 3 pegando, 4 em guarda, 5 ataque, 6 dano, 7 congelado, 8 morto...
	constexpr int32 GroupIdle = 0;
	constexpr int32 GroupWalk = 1;
	constexpr int32 GroupAttack = 5;
	constexpr int32 GroupHurt = 6;
	constexpr int32 GroupDead = 8;
	// Monstro: 0 parado, 1 andando, 2 ataque, 3 dano, 4 morte.
	constexpr int32 MobGroupAttack = 2;
	constexpr int32 MobGroupHurt = 3;
	constexpr int32 MobGroupDead = 4;

	// Materiais de sprite do Paper2D: textura "SpriteTexture" x cor do vertice, mascarados (com e sem iluminacao).
	const TCHAR* LitMaterialPath = TEXT("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial");
	const TCHAR* UnlitMaterialPath = TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial");
}

UMythRoSpriteComponent::UMythRoSpriteComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // depois do movimento e da camera do quadro
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CastShadow = false;
	bUseAsyncCooking = true;
	JobSpriteName = TEXT("초보자"); // 초보자 (Aprendiz)
}

FString UMythRoSpriteComponent::GetAssetsRoot()
{
	FString Root;
	if (GConfig && GConfig->GetString(TEXT("Myth"), TEXT("AssetsRoot"), Root, GGameIni) && !Root.IsEmpty())
	{
		return Root;
	}
	const FString Env = FPlatformMisc::GetEnvironmentVariable(TEXT("MYTH_ASSETS"));
	return Env.IsEmpty() ? FString() : FPaths::Combine(Env, TEXT("grf"));
}

void UMythRoSpriteComponent::BeginPlay()
{
	Super::BeginPlay();
	FeetRelativeLocation = GetRelativeLocation();
	ReloadSprites();

	if (bCastSpriteShadow && GetOwner())
	{
		ShadowMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("RoSpriteShadow"));
		ShadowMesh->SetupAttachment(GetAttachParent());
		ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ShadowMesh->SetRenderInMainPass(false);   // invisivel na imagem...
		ShadowMesh->SetCastShadow(true);           // ...mas projeta a sombra
		ShadowMesh->bCastDynamicShadow = true;
		ShadowMesh->RegisterComponent();
	}
	DayNight = AMythDayNightActor::Find(this);
}

void UMythRoSpriteComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (ShadowMesh)
	{
		ShadowMesh->DestroyComponent();
		ShadowMesh = nullptr;
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UMythRoSpriteComponent::UpdateShadowTransform(const FVector& Feet)
{
	if (!ShadowMesh)
	{
		return;
	}
	if (!DayNight.IsValid())
	{
		DayNight = AMythDayNightActor::Find(this);
	}
	// Plano em pe, de frente para a luz (so o yaw da luz importa): a silhueta inteira vira sombra no chao.
	FVector LightDir = DayNight.IsValid() ? DayNight->GetLightDirection() : FVector(0.3f, 0.2f, -1.f);
	LightDir.Z = 0.f;
	if (!LightDir.Normalize())
	{
		LightDir = FVector::ForwardVector; // luz a pino: qualquer orientacao serve
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, LightDir).GetSafeNormal();
	ShadowMesh->SetWorldLocationAndRotation(Feet, FRotationMatrix::MakeFromXZ(Right, FVector::UpVector).Rotator());
}

void UMythRoSpriteComponent::ReloadSprites()
{
	const FString Root = GetAssetsRoot();
	if (IsMonster())
	{
		const FString Base = FPaths::Combine(Root, TEXT("data"), TEXT("sprite"), MonsterDir, MonsterSpriteName);
		Head = FMythRoSpriteSource();
		if (!LoadSource(Body, Base + TEXT(".spr"), Base + TEXT(".act"), FString()))
		{
			UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: falha ao carregar o monstro '%s' (raiz '%s')."), *MonsterSpriteName, *Root);
		}
		return;
	}

	const FString Sex = bMale ? MaleTag : FemaleTag;
	const FString Sprite = FPaths::Combine(Root, TEXT("data"), TEXT("sprite"), HumanDir);

	const FString BodyBase = FPaths::Combine(Sprite, BodyDir, Sex, FString::Printf(TEXT("%s_%s"), *JobSpriteName, *Sex));
	const FString HeadBase = FPaths::Combine(Sprite, HeadDir, Sex, FString::Printf(TEXT("%d_%s"), HairStyle, *Sex));
	const FString Palette = HairColor > 0
		? FPaths::Combine(Root, TEXT("data"), TEXT("palette"), HairPalDir, FString::Printf(TEXT("%s%d_%s_%d.pal"), HairPalDir, HairStyle, *Sex, HairColor))
		: FString();

	const bool bBody = LoadSource(Body, BodyBase + TEXT(".spr"), BodyBase + TEXT(".act"), FString());
	const bool bHead = LoadSource(Head, HeadBase + TEXT(".spr"), HeadBase + TEXT(".act"), Palette);
	if (!bBody || !bHead)
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: falha ao carregar sprites (raiz '%s'). Corpo=%d Cabeca=%d. Configure [Myth] AssetsRoot no DefaultGame.ini."),
			*Root, bBody, bHead);
	}
}

bool UMythRoSpriteComponent::LoadSource(FMythRoSpriteSource& Source, const FString& SprPath, const FString& ActPath, const FString& PalettePath)
{
	Source = FMythRoSpriteSource();
	TArray<uint8> SprBytes, ActBytes, PalBytes;
	if (!FFileHelper::LoadFileToArray(SprBytes, *SprPath) || !FFileHelper::LoadFileToArray(ActBytes, *ActPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: arquivo nao encontrado: %s"), *SprPath);
		return false;
	}

	TArray<FColor> Palette;
	const bool bPalette = !PalettePath.IsEmpty() && FFileHelper::LoadFileToArray(PalBytes, *PalettePath) && FRoSpr::LoadPalette(PalBytes, Palette);
	if (!Source.Spr.Load(SprBytes, bPalette ? &Palette : nullptr) || !Source.Act.Load(ActBytes))
	{
		UE_LOG(LogTemp, Warning, TEXT("MythRoSprite: arquivo invalido: %s"), *SprPath);
		return false;
	}
	BuildAtlas(Source);

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bLitByScene ? LitMaterialPath : UnlitMaterialPath))
	{
		Source.Material = UMaterialInstanceDynamic::Create(Base, this);
		Source.Material->SetTextureParameterValue(TEXT("SpriteTexture"), Source.Atlas);
	}
	return Source.IsValid() && Source.Material != nullptr;
}

void UMythRoSpriteComponent::BuildAtlas(FMythRoSpriteSource& Source)
{
	// Empacota todas as imagens em prateleiras (1 px de margem), indexadas e depois RGBA.
	constexpr int32 MaxWidth = 1024;
	TArray<const FRoSprImage*> Images;
	for (const FRoSprImage& Img : Source.Spr.Indexed) { Images.Add(&Img); }
	for (const FRoSprImage& Img : Source.Spr.Rgba) { Images.Add(&Img); }

	TArray<FIntPoint> Positions;
	Positions.SetNum(Images.Num());
	int32 X = 0, Y = 0, ShelfH = 0, UsedW = 1;
	for (int32 i = 0; i < Images.Num(); ++i)
	{
		const FIntPoint Size(Images[i]->Width + 1, Images[i]->Height + 1);
		if (X + Size.X > MaxWidth)
		{
			X = 0;
			Y += ShelfH;
			ShelfH = 0;
		}
		Positions[i] = FIntPoint(X, Y);
		X += Size.X;
		UsedW = FMath::Max(UsedW, X);
		ShelfH = FMath::Max(ShelfH, Size.Y);
	}
	const int32 W = FMath::Max(UsedW, 1);
	const int32 H = FMath::Max(Y + ShelfH, 1);

	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(W * H);
	for (int32 i = 0; i < Images.Num(); ++i)
	{
		const FRoSprImage& Img = *Images[i];
		for (int32 Row = 0; Row < Img.Height; ++Row)
		{
			FMemory::Memcpy(&Pixels[(Positions[i].Y + Row) * W + Positions[i].X], &Img.Pixels[Row * Img.Width], Img.Width * sizeof(FColor));
		}
		const FBox2f UV(FVector2f(float(Positions[i].X) / W, float(Positions[i].Y) / H),
		                FVector2f(float(Positions[i].X + Img.Width) / W, float(Positions[i].Y + Img.Height) / H));
		(i < Source.Spr.Indexed.Num() ? Source.IndexedUV : Source.RgbaUV).Add(UV);
	}

	UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!Tex)
	{
		return;
	}
	Tex->Filter = TF_Nearest; // pixel art sem filtro, como no original
	Tex->SRGB = true;
	Tex->NeverStream = true;
	Tex->LODGroup = TEXTUREGROUP_Pixels2D;
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor)); // FColor ja e BGRA na memoria
	Mip.BulkData.Unlock();
	Tex->UpdateResource();
	Source.Atlas = Tex;
}

// ------------------------------------------------------------------------------------------- estado

void UMythRoSpriteComponent::SetState(EMythSpriteState NewState, bool bOneShot)
{
	if (State == EMythSpriteState::Dead && NewState != EMythSpriteState::Idle)
	{
		return;
	}
	State = NewState;
	bOneShotPlaying = bOneShot;
	StateTime = 0.f;
	PlayRate = 1.f;
}

void UMythRoSpriteComponent::PlayAttack(float AttackDuration)
{
	SetState(EMythSpriteState::Attack, true);
	if (AttackDuration > 0.f && Body.IsValid())
	{
		const FRoActAction* Action = Body.Act.Actions.IsValidIndex(GetActionIndex()) ? &Body.Act.Actions[GetActionIndex()] : nullptr;
		if (Action)
		{
			// Acelera a animacao para caber no intervalo do ASPD (nunca desacelera), como o original.
			const float Total = Action->Frames.Num() * Action->IntervalMs / 1000.f;
			PlayRate = FMath::Max(1.f, Total / AttackDuration);
		}
	}
}

void UMythRoSpriteComponent::PlayHit() { SetState(EMythSpriteState::Hit, true); }
void UMythRoSpriteComponent::Die()     { SetState(EMythSpriteState::Dead, false); }

void UMythRoSpriteComponent::Revive()
{
	State = EMythSpriteState::Idle;
	bOneShotPlaying = false;
	StateTime = 0.f;
}

void UMythRoSpriteComponent::FaceLocation(const FVector& WorldLocation)
{
	if (const AActor* Owner = GetOwner())
	{
		Direction = URagnarokDirectionHelper::CalculateSpriteDirection(WorldLocation - Owner->GetActorLocation(), GetCameraRotation(), Direction);
	}
}

int32 UMythRoSpriteComponent::GetActionIndex() const
{
	const bool bMob = IsMonster();
	int32 Group = GroupIdle;
	switch (State)
	{
	case EMythSpriteState::Idle:   Group = GroupIdle; break;
	case EMythSpriteState::Walk:   Group = GroupWalk; break;
	case EMythSpriteState::Attack: Group = bMob ? MobGroupAttack : GroupAttack; break;
	case EMythSpriteState::Hit:    Group = bMob ? MobGroupHurt : GroupHurt; break;
	case EMythSpriteState::Dead:   Group = bMob ? MobGroupDead : GroupDead; break;
	}
	return Group * 8 + static_cast<int32>(Direction);
}

int32 UMythRoSpriteComponent::GetFrameIndex(const FRoActAction& Action, bool& bOutFinished) const
{
	bOutFinished = false;
	const int32 Count = Action.Frames.Num();
	if (Count == 0)
	{
		return 0;
	}
	if (State == EMythSpriteState::Idle && !IsMonster())
	{
		return 0; // jogador parado: quadro 0 = cabeca olhando para frente (monstros animam o idle em loop)
	}
	const int32 Step = FMath::FloorToInt(StateTime * PlayRate * 1000.f / FMath::Max(Action.IntervalMs, 1.f));
	if (bOneShotPlaying || State == EMythSpriteState::Dead)
	{
		bOutFinished = Step >= Count;
		return FMath::Min(Step, Count - 1);
	}
	return Step % Count;
}

// ------------------------------------------------------------------------------------------- tick

FRotator UMythRoSpriteComponent::GetCameraRotation() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				return PC->PlayerCameraManager->GetCameraRotation();
			}
		}
	}
	return FRotator::ZeroRotator;
}

void UMythRoSpriteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetOwner();
	if (!Owner || !Body.IsValid())
	{
		return;
	}
	StateTime += DeltaTime;
	const FRotator CamRot = GetCameraRotation();

	if (State != EMythSpriteState::Dead)
	{
		const FVector Velocity = Owner->GetVelocity();
		if (bOneShotPlaying)
		{
			bool bFinished = false;
			if (Body.Act.Actions.IsValidIndex(GetActionIndex()))
			{
				GetFrameIndex(Body.Act.Actions[GetActionIndex()], bFinished);
			}
			if (bFinished)
			{
				SetState(EMythSpriteState::Idle, false);
			}
		}
		else
		{
			Direction = ComputeDirection(Velocity, CamRot);
			const bool bMoving = Velocity.Size2D() > WalkSpeedThreshold;
			StillTime = bMoving ? 0.f : StillTime + DeltaTime;
			// So volta ao Idle depois de IdleDelay parado: quinas do caminho nao reiniciam o ciclo de caminhada.
			const EMythSpriteState NewState = bMoving ? EMythSpriteState::Walk : (StillTime >= IdleDelay ? EMythSpriteState::Idle : State);
			if (NewState != State)
			{
				SetState(NewState, false);
			}
		}
	}

	UpdateBillboard(CamRot);
	RebuildMesh(CamRot);
}

ERagnarokDirection UMythRoSpriteComponent::ComputeDirection(const FVector& Velocity, const FRotator& CamRot) const
{
	if (Velocity.SizeSquared2D() < FMath::Square(WalkSpeedThreshold))
	{
		return Direction;
	}
	// Mantem a direcao atual enquanto o movimento estiver ate 22,5 + histerese graus do centro dela.
	const float MoveAngle = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X));
	const float Relative = FRotator::NormalizeAxis(MoveAngle - CamRot.Yaw);
	const float Center = URagnarokDirectionHelper::DirectionToAngle(Direction);
	if (FMath::Abs(FRotator::NormalizeAxis(Relative - Center)) <= 22.5f + DirectionHysteresisDeg)
	{
		return Direction;
	}
	return URagnarokDirectionHelper::CalculateSpriteDirection(Velocity, CamRot, Direction);
}

void UMythRoSpriteComponent::UpdateBillboard(const FRotator& CamRot)
{
	// Paralelo a tela (yaw e pitch da camera), pes fixos no ponto de apoio; levemente puxado para a camera.
	const FRotationMatrix CamMatrix(CamRot);
	const FVector CamRight = CamMatrix.GetUnitAxis(EAxis::Y);
	const FVector CamUp = CamMatrix.GetUnitAxis(EAxis::Z);
	const FVector CamForward = CamMatrix.GetUnitAxis(EAxis::X);
	SetWorldRotation(FRotationMatrix::MakeFromXZ(CamRight, CamUp).Rotator());
	if (const USceneComponent* Parent = GetAttachParent())
	{
		FVector Feet = Parent->GetComponentTransform().TransformPosition(FeetRelativeLocation);
		// A capsula sobe/desce nos degraus da malha do chao; suavizar a altura tira o "tremido" ao andar.
		const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
		if (!bFeetZInitialized || FeetHeightSmoothing <= 0.f || FMath::Abs(Feet.Z - SmoothedFeetZ) > 100.f)
		{
			SmoothedFeetZ = Feet.Z;
			bFeetZInitialized = true;
		}
		else
		{
			SmoothedFeetZ = FMath::FInterpTo(SmoothedFeetZ, Feet.Z, Dt, FeetHeightSmoothing);
		}
		Feet.Z = SmoothedFeetZ;
		SetWorldLocation(Feet - CamForward * CameraDepthOffset);
		UpdateShadowTransform(Feet);
	}
}

void UMythRoSpriteComponent::RebuildMesh(const FRotator& CamRot)
{
	const int32 ActionIndex = GetActionIndex();
	if (!Body.Act.Actions.IsValidIndex(ActionIndex))
	{
		return;
	}
	bool bFinished = false;
	const FRoActAction& BodyAction = Body.Act.Actions[ActionIndex];
	const int32 FrameIndex = GetFrameIndex(BodyAction, bFinished);
	const FRoActFrame& BodyFrame = BodyAction.Frames[FrameIndex];

	int32 Section = 0;
	FrameLocalBounds = FBox(ForceInit);
	AddLayerQuads(Body, BodyFrame, FIntPoint::ZeroValue, 0.f, Section);

	if (Head.IsValid() && Head.Act.Actions.IsValidIndex(ActionIndex) && Head.Act.Actions[ActionIndex].Frames.Num() > 0)
	{
		const FRoActAction& HeadAction = Head.Act.Actions[ActionIndex];
		const FRoActFrame& HeadFrame = HeadAction.Frames[FrameIndex % HeadAction.Frames.Num()];
		// A cabeca encaixa no corpo pela diferenca entre as ancoras dos dois quadros.
		FIntPoint Offset = FIntPoint::ZeroValue;
		if (BodyFrame.Anchors.Num() > 0 && HeadFrame.Anchors.Num() > 0)
		{
			Offset = BodyFrame.Anchors[0] - HeadFrame.Anchors[0];
		}
		AddLayerQuads(Head, HeadFrame, Offset, 2.f, Section);
	}

	for (int32 i = Section; i < SectionsCreated; ++i)
	{
		SetMeshSectionVisible(i, false);
		if (ShadowMesh)
		{
			ShadowMesh->SetMeshSectionVisible(i, false);
		}
	}
}

void UMythRoSpriteComponent::AddLayerQuads(const FMythRoSpriteSource& Source, const FRoActFrame& Frame, const FIntPoint& Offset, float DepthBase, int32& Section)
{
	// No espaco local (ver UpdateBillboard): X = direita da tela, Z = cima da tela, Y = Z x X = -Forward (para a camera).
	const float S = UnitsPerPixel;
	for (int32 LayerIndex = 0; LayerIndex < Frame.Layers.Num(); ++LayerIndex)
	{
		const FRoActLayer& L = Frame.Layers[LayerIndex];
		const FRoSprImage* Img = Source.Spr.Find(L.SprIndex, L.SprType);
		const TArray<FBox2f>& UVs = L.SprType == 1 ? Source.RgbaUV : Source.IndexedUV;
		if (!Img || !UVs.IsValidIndex(L.SprIndex))
		{
			continue;
		}
		const FBox2f UV = UVs[L.SprIndex];
		const float HalfW = Img->Width * L.Scale.X * 0.5f;
		const float HalfH = Img->Height * L.Scale.Y * 0.5f;
		const float CenterX = L.Offset.X + Offset.X;
		const float CenterY = L.Offset.Y + Offset.Y;
		const float Rad = FMath::DegreesToRadians(static_cast<float>(L.RotationDeg));
		const float C = FMath::Cos(Rad), Sn = FMath::Sin(Rad);
		// Cada camada fica um pouco mais perto da camera que a anterior (evita z-fighting).
		const float Depth = DepthBase + LayerIndex * 0.5f;

		// Cantos em pixels (Y para baixo): sup-esq, sup-dir, inf-dir, inf-esq.
		const FVector2f Corners[4] = { {-HalfW, -HalfH}, {HalfW, -HalfH}, {HalfW, HalfH}, {-HalfW, HalfH} };
		const float U0 = L.bMirror ? UV.Max.X : UV.Min.X;
		const float U1 = L.bMirror ? UV.Min.X : UV.Max.X;
		const FVector2D CornerUV[4] = { {U0, UV.Min.Y}, {U1, UV.Min.Y}, {U1, UV.Max.Y}, {U0, UV.Max.Y} };

		TArray<FVector> Verts;
		TArray<FVector2D> Tex;
		TArray<FColor> Colors;
		TArray<FVector> Normals;
		for (int32 k = 0; k < 4; ++k)
		{
			const float Px = CenterX + Corners[k].X * C - Corners[k].Y * Sn;
			const float Py = CenterY + Corners[k].X * Sn + Corners[k].Y * C;
			Verts.Add(FVector(Px * S, Depth, -Py * S));
			FrameLocalBounds += Verts.Last();
			Tex.Add(CornerUV[k]);
			Colors.Add(L.Color);
			Normals.Add(FVector(0.f, 1.f, 0.f));
		}

		UMaterialInterface* Mat = Source.Material;
		// Dois triangulos nos dois sentidos: o quad aparece mesmo se o winding ficar invertido.
		static const TArray<int32> Tris = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
		if (Section < SectionsCreated)
		{
			UpdateMeshSection(Section, Verts, Normals, Tex, Colors, TArray<FProcMeshTangent>());
			SetMeshSectionVisible(Section, true);
			if (ShadowMesh)
			{
				ShadowMesh->UpdateMeshSection(Section, Verts, Normals, Tex, Colors, TArray<FProcMeshTangent>());
				ShadowMesh->SetMeshSectionVisible(Section, true);
			}
		}
		else
		{
			CreateMeshSection(Section, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
			if (ShadowMesh)
			{
				ShadowMesh->CreateMeshSection(Section, Verts, Tris, Normals, Tex, Colors, TArray<FProcMeshTangent>(), false);
			}
			++SectionsCreated;
		}
		if (GetMaterial(Section) != Mat)
		{
			SetMaterial(Section, Mat);
			if (ShadowMesh)
			{
				ShadowMesh->SetMaterial(Section, Mat); // mascarado: a sombra tem o recorte do sprite
			}
		}
		++Section;
	}
}
