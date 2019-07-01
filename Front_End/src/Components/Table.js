import React,{Component} from 'react';

class Table extends Component{
    constructor() {
        super();
        this.state={

        }
    }

    render() {
        console.log(this.props.head);
        return(
            <table>
                <thead>
                    <tr>
                        {this.props.head.map((a,i)=>
                                <td key={i}> {a}</td>
                        )}
                    </tr>
                </thead>
                <tbody>
                    {this.props.contents.map((a,i) => 
                        <tr key={i}>
                            {a.map((b,j)=> 
                                <td key={j}>
                                    {b}
                                </td>
                                )
                            }
                        </tr>
                        )
                    }    
                </tbody>
            </table>
        );
    }

}

export default Table;