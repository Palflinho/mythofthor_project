-- Segunda conta de testes do Claude (testes com dois jogadores: grupo, cochicho, troca)
-- Uso: wsl docker exec -i mythofthor-db mariadb -u ragnarok -pragnarok ragnarok < server/init_claude_account2.sql
INSERT INTO login (account_id, userid, user_pass, sex, email, group_id, birthdate)
VALUES (2000003, 'mythclaude2', 'mythclaude123', 'M', 'mythclaude2@mythofthor.local', 99, '2000-01-01')
ON DUPLICATE KEY UPDATE user_pass='mythclaude123', group_id=99;
-- personagem: copia da ThorTester (nivel 99), com outro id e nome
DROP TEMPORARY TABLE IF EXISTS tmp_claude_char2;
CREATE TEMPORARY TABLE tmp_claude_char2 AS SELECT * FROM `char` WHERE char_id = 150000;
UPDATE tmp_claude_char2 SET char_id = 150003, account_id = 2000003, name = 'ClaudeTester2', char_num = 0;
REPLACE INTO `char` SELECT * FROM tmp_claude_char2;
