-- Conta dedicada para testes automáticos de rede e robôs de teste (Task 004)
-- Uso: wsl docker exec -i mythofthor-db mariadb -u ragnarok -pragnarok ragnarok < server/init_bot_account.sql

INSERT INTO login (account_id, userid, user_pass, sex, email, group_id, birthdate) 
VALUES (2000001, 'mythbot', 'mythbot', 'M', 'mythbot@mythofthor.local', 99, '2000-01-01') 
ON DUPLICATE KEY UPDATE user_pass='mythbot', group_id=99;

INSERT INTO `char` (
    char_id, account_id, char_num, name, class, 
    base_level, job_level, max_hp, hp, max_sp, sp, 
    str, agi, vit, `int`, dex, luk, 
    last_map, last_x, last_y, save_map, save_x, save_y, sex
) VALUES (
    150001, 2000001, 0, 'MythBot', 0, 
    1, 1, 40, 40, 11, 11, 
    1, 1, 1, 1, 1, 1, 
    'prontera', 156, 180, 'prontera', 156, 180, 'M'
) ON DUPLICATE KEY UPDATE 
    last_map='prontera', last_x=156, last_y=180, save_map='prontera', save_x=156, save_y=180;
