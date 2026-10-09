#include "World/MythGroundItem.h"

#include "Sprites/MythRoSpriteComponent.h"
#include "UI/MythRoClientTables.h"

#include "Components/BoxComponent.h"

namespace
{
	constexpr float ItemDropDuration = 0.45f; // queda + quique
	constexpr float ItemDropHeight = 60.f;
}

AMythGroundItem::AMythGroundItem()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sprite = CreateDefaultSubobject<UMythRoSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Root);
	Sprite->MonsterSpriteFolder = TEXT("아이템"); // 아이템
	Sprite->bPlayActSounds = false;
	Sprite->bBlobShadow = false;       // no cliente original o item no chao nao tem a sombra redonda
	Sprite->bCastSpriteShadow = false;

	// Area de clique sobre o sprite (que fica paralelo a tela), como nos monstros
	ClickBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBox"));
	ClickBox->SetupAttachment(Sprite);
	ClickBox->SetBoxExtent(FVector(22.f, 10.f, 22.f));
	ClickBox->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	ClickBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AMythGroundItem::Setup(int64 InNetID, int32 InItemID, int32 InAmount, bool bJustDropped)
{
	NetID = InNetID;
	ItemID = InItemID;
	Amount = FMath::Max(1, InAmount);
	FString Resource = FMythRoClientTables::ItemResource(ItemID);
	Sprite->MonsterSpriteName = Resource.IsEmpty() ? TEXT("사과") : Resource; // 사과 (maca) se faltar na tabela
	RestLocation = GetActorLocation();
	DropTime = bJustDropped ? 0.f : -1.f;
}

FString AMythGroundItem::GetLabel() const
{
	FString Name = FMythRoClientTables::ItemName(ItemID);
	if (Name.IsEmpty())
	{
		Name = FString::Printf(TEXT("Item %d"), ItemID);
	}
	return Amount > 1 ? FString::Printf(TEXT("%s: %d"), *Name, Amount) : Name;
}

void AMythGroundItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DropTime < 0.f)
	{
		return;
	}
	// Item recem-caido: sobe e cai com um quique, como o drop do cliente original
	DropTime += DeltaSeconds;
	const float T = FMath::Clamp(DropTime / ItemDropDuration, 0.f, 1.f);
	const float Arc = 4.f * T * (1.f - T);                       // parabola 0..1..0
	const float Bounce = T > 0.7f ? 0.15f * FMath::Sin((T - 0.7f) / 0.3f * PI) : 0.f;
	SetActorLocation(RestLocation + FVector(0.f, 0.f, ItemDropHeight * (Arc + Bounce)));
	if (T >= 1.f)
	{
		SetActorLocation(RestLocation);
		DropTime = -1.f;
	}
}
