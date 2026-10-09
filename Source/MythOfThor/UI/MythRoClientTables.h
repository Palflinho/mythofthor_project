#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/**
 * Tabelas do cliente original (iteminfo / skillinfo), extraidas do data.grf para Config/items.json e
 * Config/skills.json (tools/extract_client_tables.py). Carregadas sob demanda, uma unica vez.
 * O servidor manda so IDs; nome, descricao e icone sao responsabilidade do cliente, como no original.
 */
struct MYTHOFTHOR_API FMythRoClientTables
{
	/** Nome exibido do item (ex.: 501 -> "Pocao Vermelha"). Vazio se o ID nao existir. */
	static FString ItemName(int32 ItemID);

	/** Icone 24x24 do item (texture/유저인터페이스/item/<resource_name>.bmp). */
	static UTexture2D* ItemIcon(int32 ItemID);

	/** Nome exibido da habilidade (ex.: 5 -> "Golpe Fulminante"). */
	static FString SkillName(int32 SkillID);

	/** Icone da habilidade (texture/유저인터페이스/item/<aegis_name minusculo>.bmp). */
	static UTexture2D* SkillIcon(int32 SkillID);
};
