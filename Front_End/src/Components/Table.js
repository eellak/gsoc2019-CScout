import React,{Component} from 'react';
import './Table.css';

class Table extends Component{
    constructor() {
        super();
        this.state={

        }
    }

    render() {
        console.log(this.props.head);
        return(
            <table className="datatable">
                <thead>
                    <tr>
                        {this.props.head.map((a,i)=>
                                <td key={i} style={{fontWeight:'bold'}}> {a}</td>
                        )}
                    </tr>
                </thead>
                <tbody>
                    {this.props.contents.map((a,i) => 
                        <tr key={i}>
                            {a.map((b,j)=>     
                                <td key={j} style={(j===0)?{textAlign: 'left'}:{textAlign:'right'}}>
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