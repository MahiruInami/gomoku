import json

with open('Templates.json') as json_file:
    data = json.load(json_file)
    template_id = 0
    patterns_mapping = {}
    for template in data['templates']:
        patterns_mapping[template['uniqueId']] = template['pattern']
    miai_points = {}
    for points in data['miai_points']:
        print("-")
        print("Miai id: ", points['miaiId'])
        for pattern_id in points['patterns']:
            print(patterns_mapping[pattern_id], "      ", pattern_id)
            miai_points[pattern_id] = points['miaiId']
    for template in data['templates']:
        pattern = template["pattern"]
        pattern = pattern.replace('E', '0')
        while pattern[-1] =='0':
            pattern = pattern[:-1]
        while pattern[0] =='0':
            pattern = pattern[1:]
        index = pattern.find("*")
        shift = index
        attack_shift = 0
        while shift < len(pattern):
            if pattern[shift] == '1':
                attack_shift = index - shift
            shift += 1
        if attack_shift == 0:
            shift = index
            while shift >= 0:
                if pattern[shift] == '1':
                    attack_shift = index - shift
                    break
                shift -= 1
        attack_pattern = template['pattern']
        attack_pattern = attack_pattern.replace('E', '0')
        attack_pattern = attack_pattern.replace('*', '0')
        while attack_pattern[-1] =='0':
            attack_pattern = attack_pattern[:-1]
        empty_pattern = template['pattern']
        empty_pattern = empty_pattern.replace('1', '2')
        empty_pattern = empty_pattern.replace('0', '1')
        empty_pattern = empty_pattern.replace('2', '0')
        empty_pattern = empty_pattern.replace('E', '0')
        while empty_pattern[-1] =='0':
            empty_pattern = empty_pattern[:-1]
        while empty_pattern[0] =='0':
            empty_pattern = empty_pattern[1:]
        index = empty_pattern.find("*")
        shift = index
        empty_shift = 0
        while shift < len(empty_pattern):
            if empty_pattern[shift] == '1':
                empty_shift = index - shift
            shift += 1
        if empty_shift == 0:
            shift = index
            while shift >= 0:
                if empty_pattern[shift] == '1':
                    empty_shift = index - shift
                    break
                shift -= 1
        empty_pattern = empty_pattern.replace('*', '1')
        enemy_pattern = template['pattern']
        enemy_pattern = enemy_pattern.replace('1', '0')
        enemy_pattern = enemy_pattern.replace('E', '1')
        while len(enemy_pattern) > 0 and enemy_pattern[-1] =='0':
            enemy_pattern = enemy_pattern[:-1]
        while len(enemy_pattern) > 0 and enemy_pattern[0] =='0':
            enemy_pattern = enemy_pattern[1:]
        index = enemy_pattern.find("*")
        shift = index
        enemy_shift = 0
        while shift < len(enemy_pattern):
            if enemy_pattern[shift] == '1':
                enemy_shift = index - shift
            shift += 1
        if enemy_shift == 0:
            shift = index
            while shift >= 0:
                if enemy_pattern[shift] == '1':
                    enemy_shift = index - shift
                    break
                shift -= 1
        enemy_pattern = enemy_pattern.replace('*', '0')
        if len(enemy_pattern) == 1:
            enemy_pattern = ''
        while len(enemy_pattern) > 0 and enemy_pattern[-1] =='0':
            enemy_pattern = enemy_pattern[:-1]
        while len(enemy_pattern) > 0 and enemy_pattern[0] =='0':
            enemy_pattern = enemy_pattern[1:]
        if len(enemy_pattern) == 0:
            enemy_pattern = '0'
        defence_index = [0]
        defence_priority = [template['priority']]
        defence_str = template['defence_pattern']
        defence_start_index = defence_str.find('*')
        shift = 0
        while defence_start_index + shift >= 0:
            if defence_str[defence_start_index + shift] == '1':
                defence_index.append(-shift)
                defence_priority.append(template['priority'])
            shift -= 1
        shift = 0
        while defence_start_index + shift < len(defence_str):
            if defence_str[defence_start_index + shift] == '1':
                defence_index.append(-shift)
                defence_priority.append(template['priority'])
            shift += 1
        defence_out = f"{{{str(defence_index).strip('[]')}}}"
        defence_priority_out = f"{{{str(defence_priority).strip('[]')}}}"
        print(f"{{{template_id}, {int(attack_pattern, 2)}, {int(empty_pattern, 2)}, {int(enemy_pattern, 2)}, {attack_shift}, {empty_shift}, {enemy_shift}, {template['priority']}, {miai_points[template['uniqueId']]}, {defence_out}, {defence_priority_out}}},")
        template_id += 1