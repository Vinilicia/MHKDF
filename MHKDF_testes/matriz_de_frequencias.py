import numpy as np
import matplotlib.pyplot as plt

def read_data_from_file(filename):
    with open(filename, 'r') as file:
        content = file.read().strip()
    numbers = [int(num) for num in content.split()]
    
    # Assumir que os primeiros dois números são o número de linhas e colunas
    rows = numbers[0]
    cols = numbers[1]
    
    # Criar a matriz inicializada com zeros
    matrix = [[0 for _ in range(cols)] for _ in range(rows)]
    
    # Preencher a matriz com base nos pares de números subsequentes
    for i in range(2, len(numbers), 2):
        num1 = numbers[i]
        num2 = numbers[i + 1]
        if 0 <= num1 < rows and 0 <= num2 < cols:  # Verificar se o índice está dentro dos limites
            matrix[num1][num2] += 1

    return np.array(matrix)

def print_colour_matrices(matrix1, matrix2, title1, title2):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 6))  # Cria uma figura com dois subplots lado a lado
    
    # Plotar a primeira matriz
    c1 = ax1.imshow(matrix1, cmap='viridis', interpolation='nearest')
    ax1.set_title(title1)
    fig.colorbar(c1, ax=ax1)

    # Plotar a segunda matriz
    c2 = ax2.imshow(matrix2, cmap='viridis', interpolation='nearest')
    ax2.set_title(title2)
    fig.colorbar(c2, ax=ax2)

    plt.tight_layout()
    plt.show()
    
    # Salvar a imagem
    filename = 'Modificados e Acessados.png'
    fig.savefig(filename)  
    return filename

def count_values(matrix):
    zeros = np.sum(matrix == 0)
    non_zeros = np.sum(matrix != 0)
    return zeros, non_zeros

filename1 = 'acessados.txt'
matrix1 = read_data_from_file(filename1)

filename2 = 'modificados.txt'
matrix2 = read_data_from_file(filename2)

zeros1, non_zeros1 = count_values(matrix1)
zeros2, non_zeros2 = count_values(matrix2)

print(f'Para a matriz 1 ({filename1}):')
print(f'Valores iguais a zero: {zeros1}')
print(f'Valores diferentes de zero: {non_zeros1}')

print(f'Para a matriz 2 ({filename2}):')
print(f'Valores iguais a zero: {zeros2}')
print(f'Valores diferentes de zero: {non_zeros2}')


image_filename = print_colour_matrices(matrix1, matrix2, 'Acessados', 'Modificados')
print(f'Combined image saved as {image_filename}')
