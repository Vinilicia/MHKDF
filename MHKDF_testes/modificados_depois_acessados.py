import numpy as np
import matplotlib.pyplot as plt

class Matriz:
    def __init__(self, cont=0, tempo=int(1e9), ok=0):  # Usando um valor grande para representar infinito
        self.cont = cont
        self.tempo = tempo
        self.ok = ok

def read_data_from_file(filename):
    with open(filename, 'r') as file:
        content = file.read().strip()
    numbers = [int(num) for num in content.split()]
    
    # Assumir que os primeiros dois números são o número de linhas e colunas
    rows = numbers[0]
    cols = numbers[1]
    
    # Criar a matriz inicializada com objetos Matriz
    matrix = [[Matriz() for _ in range(cols)] for _ in range(rows)]
    
    # Preencher a matriz com base nos pares de números subsequentes
    for i in range(2, len(numbers), 3):
        num1 = numbers[i]
        num2 = numbers[i + 1]
        temp = numbers[i + 2]
        if 0 <= num1 < rows and 0 <= num2 < cols:  # Verificar se o índice está dentro dos limites
            matrix[num1][num2].cont += 1
            matrix[num1][num2].tempo = temp

    return matrix

def read_data_from_file2(filename, matrix):
    with open(filename, 'r') as file:
        content = file.read().strip()
    numbers = [int(num) for num in content.split()]
    
    # Assumir que os primeiros dois números são o número de linhas e colunas
    rows = numbers[0]
    cols = numbers[1]
    
    # Preencher a matriz com base nos pares de números subsequentes
    for i in range(2, len(numbers), 3):
        num1 = numbers[i]
        num2 = numbers[i + 1]
        temp = numbers[i + 2]
        if 0 <= num1 < rows and 0 <= num2 < cols:  # Verificar se o índice está dentro dos limites
            if matrix[num1][num2].tempo < temp:
               matrix[num1][num2].ok = 1 

    return matrix

def convert_matrix_to_array(matrix, attribute='ok'):
    """Converter a matriz de objetos para uma matriz NumPy de valores numéricos."""
    rows = len(matrix)
    cols = len(matrix[0])
    array = np.zeros((rows, cols), dtype=int)
    for i in range(rows):
        for j in range(cols):
            array[i, j] = getattr(matrix[i][j], attribute)
    return array

def print_colour_matrices(matrix, title):
    fig, ax = plt.subplots(figsize=(6, 6))  # Cria uma figura com um subplot
    
    # Converter a matriz de objetos para uma matriz NumPy de valores numéricos
    numeric_matrix = convert_matrix_to_array(matrix)

    # Contar valores na matriz
    zeros, non_zeros = count_values(numeric_matrix)

    print(f'Para a matriz ({title}):')
    print(f'Valores iguais a zero: {zeros}')
    print(f'Valores diferentes de zero: {non_zeros}')

    # Plotar a matriz
    c = ax.imshow(numeric_matrix, cmap='viridis', interpolation='nearest')
    ax.set_title(title)
    fig.colorbar(c, ax=ax)

    plt.tight_layout()
    plt.show()
    
    # Salvar a imagem
    filename = 'Modificados depois Acessados.png'
    fig.savefig(filename)  
    return filename

def count_values(matrix):
    """Contar valores iguais a zero e diferentes de zero na matriz."""
    zeros = np.sum(matrix == 0)
    non_zeros = np.sum(matrix != 0)
    return zeros, non_zeros

filename1 = 'modificados.txt'
matrix = read_data_from_file(filename1)

filename2 = 'acessados.txt'
matrix = read_data_from_file2(filename2, matrix)

image_filename = print_colour_matrices(matrix, 'Modificados')
print(f'Combined image saved as {image_filename}')
