def main():
    with open('/home/cooper/.profile', 'r') as f:
        s = f.read().split('\n')
    for i in range(len(s)):
        if s[i].startswith('sleep'):
            s[i] = 'sleep 45'
    with open('/home/cooper/.profile', 'w') as f:
        f.write('\n'.join(s))
    print('changes made, please turn off device')

if __name__ == '__main__':
    main()
