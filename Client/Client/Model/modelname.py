import os

def save_bin_filenames_without_extension(directory, output_file):
    bin_files = [f for f in os.listdir(directory)
                 if os.path.isfile(os.path.join(directory, f)) and f.endswith('.bin')]

    # 확장자 제거 후 저장
    with open(output_file, 'w', encoding='utf-8') as f:
        for file_name in bin_files:
            name_without_ext = os.path.splitext(file_name)[0]  # 확장자 제거
            f.write(name_without_ext + '\n')

# 사용 예시
directory_path = 'C:/VisualStudio/ZombieStrike/Client/Client/Model'         # 대상 디렉토리
output_txt_path = 'C:/VisualStudio/ZombieStrike/Client/Client/Model/ModelList.txt'  # 결과를 저장할 경로

save_bin_filenames_without_extension(directory_path, output_txt_path)