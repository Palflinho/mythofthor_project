-- Conta dedicada aos testes automaticos do Claude (nao mexe na mythtest do usuario nem na mythbot do Antigravity)
-- Uso: wsl docker exec -i mythofthor-db mariadb -u ragnarok -pragnarok ragnarok < server/init_claude_account.sql
INSERT INTO login (account_id, userid, user_pass, sex, email, group_id, birthdate)
VALUES (2000002, 'mythclaude', 'mythclaude123', 'M', 'mythclaude@mythofthor.local', 99, '2000-01-01')
ON DUPLICATE KEY UPDATE user_pass='mythclaude123', group_id=99;
-- personagem: copia da ThorTester (nivel 99), com outro id e nome
DROP TEMPORARY TABLE IF EXISTS tmp_claude_char;
CREATE TEMPORARY TABLE tmp_claude_char AS SELECT * FROM `char` WHERE char_id = 150000;
UPDATE tmp_claude_char SET char_id = 150002, account_id = 2000002, name = 'ClaudeTester', char_num = 0;
REPLACE INTO `char` SELECT * FROM tmp_claude_char;
