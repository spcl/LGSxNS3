import plotly.express as px
import pandas as pd
from pathlib import Path
import plotly.graph_objs as go
 
# RTT Data
colnames=['Time', 'RTT'] 
df = pd.read_csv('rtt/rtt_veno0.txt', names=colnames, header=None, index_col=False, sep=',')
name = ['0'] * df.shape[0]
df = df.assign(Node=name)

pathlist = Path('rtt').glob('**/*.txt')
for files in sorted(pathlist):
    path_in_str = str(files)
    if (int(list(filter(str.isdigit, path_in_str))[0]) == 0):
        continue
    temp_df = pd.read_csv(path_in_str, names=colnames, header=None, index_col=False, sep=',')
    name = [str(int(list(filter(str.isdigit, path_in_str))[0]))] * temp_df.shape[0]
    temp_df = temp_df.assign(Node=name)
    df = pd.concat([df, temp_df])

# Cwd Data
colnames=['Time', 'Congestion Window'] 
df2 = pd.read_csv('cwd/my0.txt', names=colnames, header=None, index_col=False, sep=',')
name = ['0'] * df2.shape[0]
df2 = df2.assign(Node=name)
print(df2.shape[0])
df2.drop_duplicates('Time', inplace = True)
print(df2.shape[0])

pathlist = Path('cwd').glob('**/*.txt')
for files in sorted(pathlist):
    path_in_str = str(files)
    print(path_in_str)
    print(int(list(filter(str.isdigit, path_in_str))[0]))
    if (int(list(filter(str.isdigit, path_in_str))[0]) == 0):
        continue
    temp_df2 = pd.read_csv(path_in_str, names=colnames, header=None, index_col=False, sep=',')
    name = [str(int(list(filter(str.isdigit, path_in_str))[0]))] * temp_df2.shape[0]
    temp_df2 = temp_df2.assign(Node=name)
    df2 = pd.concat([df2, temp_df2])
    df2.drop_duplicates('Time', inplace = True)

# Plotting Now
'''fig = go.Figure()
fig = go.Figure(fig.add_traces(
                 data=px.line(df, x='Time', y='RTT',
                              line_group="Node", color='Node', markers=True)._data))
fig.add_trace(go.Scatter(x = df2['Time'], y = df2['Congestion Window']))
fig.update_layout(title='RTT Over Time', showlegend=True)
fig.show()'''
print(df2)
fig = go.Figure()
fig.add_traces(
                 data=px.line(df, x='Time', y='RTT',
                              line_group="Node", color='Node', markers=True)._data)
fig.add_traces(
                 data=px.line(df2, x='Time', y='Congestion Window',
                              line_group="Node", color='Node')._data)
fig.update_layout(title='RTT Over Time', showlegend=True)

fig.show()
