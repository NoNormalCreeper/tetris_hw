import random

def generate_input_file(filename="input.txt", num_lines=1000000):
    """
    Generates a file with a specified number of lines,
    each containing a random character from a predefined set.

    Args:
        filename (str): The name of the file to create.
        num_lines (int): The number of lines to generate.
    """
    characters = ['I', 'O', 'T', 'J', 'L', 'S', 'Z']
    try:
        with open(filename, 'w') as f:
            for _ in range(num_lines):
                random_char = random.choice(characters)
                f.write(random_char + '\n')
            f.write('X\n')
        print(f"Successfully generated {num_lines} lines in {filename}")
    except IOError:
        print(f"Error: Could not write to file {filename}")

if __name__ == "__main__":
    generate_input_file()