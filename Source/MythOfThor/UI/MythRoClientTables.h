#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** Descricao do item do itemInfo.lua do cliente (Config/item_info.json, tools/gen_item_info.py). */
struct FMythRoItemInfo
{
	FString Name;
	TArray<FString> Desc;
	FString UnidentifiedName;
	TArray<FString> UnidentifiedDesc;
	FString Resource;
	int32 Slots = 0;
};

/**
 * Tabelas do cliente original (iteminfo / skillinfo), extraidas do data.grf para Config/items.json e
 * Config/skills.json (tools/extract_client_tables.py). Carregadas sob demanda, uma unica vez.
 * O servidor manda so IDs; nome, descricao e icone sao responsabilidade do cliente, como no original.
 */
struct MYTHOFTHOR_API FMythRoClientTables
{
	/** Nome exibido do item (ex.: 501 -> "Pocao Vermelha"). Vazio se o ID nao existir. */
	static FString ItemName(int32 ItemID);

	/** Nome, descricao e slots do item como no cliente (itemInfo.lua). Nulo se nao existir. */
	static const FMythRoItemInfo* ItemInfo(int32 ItemID);

	/** Ilustracao grande do item (texture/유저인터페이스/collection/<recurso>.bmp; cartas: cardbmp). */
	static UTexture2D* ItemIllustration(int32 ItemID);

	/** Icone 24x24 do item (texture/유저인터페이스/item/<resource_name>.bmp). */
	static UTexture2D* ItemIcon(int32 ItemID);

	/** Nome do recurso do item (sprite no chao: data/sprite/아이템/<nome>.spr; icone: item/<nome>.bmp). */
	static FString ItemResource(int32 ItemID);

	/** Raridade visual do item (0 comum .. 4 lendario), de Config/item_rarity.json (tools/gen_item_rarity.py). */
	static int32 ItemRarity(int32 ItemID);

	/** Cor da raridade (nome no chao e aura), no estilo Diablo: branco, azul, amarelo, roxo, laranja. */
	static FLinearColor RarityColor(int32 Rarity);

	/** Nome exibido da habilidade (ex.: 5 -> "Golpe Fulminante"). */
	static FString SkillName(int32 SkillID);

	/** Icone da habilidade (texture/유저인터페이스/item/<aegis_name minusculo>.bmp). */
	static UTexture2D* SkillIcon(int32 SkillID);
};
